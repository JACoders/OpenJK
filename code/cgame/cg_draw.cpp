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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_headers.h"

#include "cg_media.h"
#include "../game/objectives.h"
#include "../game/g_vehicles.h"

extern vmCvar_t	cg_debugHealthBars;

extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );

void CG_DrawIconBackground(void);
void CG_DrawMissionInformation( void );
void CG_DrawInventorySelect( void );
void CG_DrawForceSelect( void );
qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, int *x, int *y);
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y);

extern float g_crosshairEntDist;
extern int g_crosshairSameEntTime;
extern int g_crosshairEntNum;
extern int g_crosshairEntTime;
qboolean cg_forceCrosshair = qfalse;

// bad cheating
extern int g_rocketLockEntNum;
extern int g_rocketLockTime;
extern int g_rocketSlackTime;

vec3_t	vfwd;
vec3_t	vright;
vec3_t	vup;
vec3_t	vfwd_n;
vec3_t	vright_n;
vec3_t	vup_n;
int		infoStringCount;

int cgRageTime = 0;
int cgRageFadeTime = 0;
float cgRageFadeVal = 0;

int cgRageRecTime = 0;
int cgRageRecFadeTime = 0;
float cgRageRecFadeVal = 0;

int cgAbsorbTime = 0;
int cgAbsorbFadeTime = 0;
float cgAbsorbFadeVal = 0;

int cgProtectTime = 0;
int cgProtectFadeTime = 0;
float cgProtectFadeVal = 0;
//===============================================================


/*
================
CG_DrawMessageLit
================
*/
static void CG_DrawMessageLit(centity_t *cent,int x,int y)
{
	cgi_R_SetColor(colorTable[CT_WHITE]);

	if (cg.missionInfoFlashTime	> cg.time )
	{
		if (!((cg.time / 600 ) & 1))
		{
			if (!cg.messageLitActive)
			{
				/*

				kef 4/16/03 --	as fun as this was, its time has passed. I will, however, hijack this cvar at James'
								recommendation and use it for another nefarious purpose

				if (cg_neverHearThatDumbBeepingSoundAgain.integer == 0)
				{
					cgi_S_StartSound( NULL, 0, CHAN_AUTO, cgs.media.messageLitSound );
				}
				*/
				cg.messageLitActive = qtrue;
			}

			cgi_R_SetColor(colorTable[CT_HUD_RED]);
			CG_DrawPic( x + 33,y + 41, 16,16, cgs.media.messageLitOn);
		}
		else
		{
			cg.messageLitActive = qfalse;
		}
	}

	cgi_R_SetColor(colorTable[CT_WHITE]);
	CG_DrawPic( x + 33,y + 41, 16,16, cgs.media.messageLitOff);

}

/*
================
CG_DrawForcePower

Draw the force power graphics (tics) and the force power numeric amount. Any tics that are partial will
be alphaed out.
================
*/
static void CG_DrawForcePower(const centity_t *cent,const int xPos,const int yPos)
{
	int			i;
	qboolean	flash=qfalse;
	vec4_t		calcColor;
	float		value,extra=0,inc,percent;

	if ( !cent->gent->client->ps.forcePowersKnown )
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.forceHUDTotalFlashTime > cg.time )
	{
		flash = qtrue;
		if (cg.forceHUDNextFlashTime < cg.time)	
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			cgi_S_StartSound( NULL, 0, CHAN_AUTO, cgs.media.noforceSound );
			if (cg.forceHUDActive)
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}

		}
	}
	else	// turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

	// I left the funtionality for flashing in because I might be needed yet.
//	if (!cg.forceHUDActive)
//	{
//		return;
//	}

	inc = (float)  cent->gent->client->ps.forcePowerMax / MAX_HUD_TICS;
	value = cent->gent->client->ps.forcePower;
	if ( value > cent->gent->client->ps.forcePowerMax )
	{//supercharged with force
		extra = value - cent->gent->client->ps.forcePowerMax;
		value = cent->gent->client->ps.forcePowerMax;
	}

	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		if ( extra )
		{//supercharged
			memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
			percent = 0.75f + (sin( cg.time * 0.005f )*((extra/cent->gent->client->ps.forcePowerMax)*0.25f));
			calcColor[0] *= percent;
			calcColor[1] *= percent;
			calcColor[2] *= percent;
		}
		else if ( value <= 0 )	// no more
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			if (flash)
			{
				memcpy(calcColor,  colorTable[CT_RED], sizeof(vec4_t));
			}
			else 
			{
				memcpy(calcColor,  colorTable[CT_WHITE], sizeof(vec4_t));
			}

			percent = value / inc;
			calcColor[3] = percent;
		}
		else
		{
			if (flash)
			{
				memcpy(calcColor,  colorTable[CT_RED], sizeof(vec4_t));
			}
			else 
			{
				memcpy(calcColor,  colorTable[CT_WHITE], sizeof(vec4_t));
			}
		}

		cgi_R_SetColor( calcColor);
		CG_DrawPic( forceTics[i].xPos, 
			forceTics[i].yPos, 
			forceTics[i].width, 
			forceTics[i].height, 
			forceTics[i].background );

		value -= inc;
	}

	if (flash)
	{
		cgi_R_SetColor( colorTable[CT_RED] );	
	}
	else
	{
		cgi_R_SetColor( otherHUDBits[OHB_FORCEAMOUNT].color );	
	}

	// Print force numeric amount
	CG_DrawNumField (
		otherHUDBits[OHB_FORCEAMOUNT].xPos, 
		otherHUDBits[OHB_FORCEAMOUNT].yPos, 
		3, 
		cent->gent->client->ps.forcePower, 
		otherHUDBits[OHB_FORCEAMOUNT].width, 
		otherHUDBits[OHB_FORCEAMOUNT].height, 
		NUM_FONT_SMALL,
		qfalse);
}

/*
================
CG_DrawSaberStyle

If the weapon is a light saber (which needs no ammo) then draw a graphic showing
the saber style (fast, medium, strong)
================
*/
static void CG_DrawSaberStyle(const centity_t	*cent,const int xPos,const int yPos)
{
	int index;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon != WP_SABER || !cent->gent )
	{
		return;
	}

	cgi_R_SetColor( colorTable[CT_WHITE] );

	if ( !cg.saberAnimLevelPending && cent->gent->client )
	{//uninitialized after a loadgame, cheat across and get it
		cg.saberAnimLevelPending = cent->gent->client->ps.saberAnimLevel;
	}

	// don't need to draw ammo, but we will draw the current saber style in this window
	if (cg.saberAnimLevelPending == SS_FAST
		|| cg.saberAnimLevelPending == SS_TAVION )
	{
		index = OHB_SABERSTYLE_FAST;
	}
	else if (cg.saberAnimLevelPending == SS_MEDIUM
		|| cg.saberAnimLevelPending == SS_DUAL
		|| cg.saberAnimLevelPending == SS_STAFF )
	{
		index = OHB_SABERSTYLE_MEDIUM;
	}
	else 
	{
		index = OHB_SABERSTYLE_STRONG;
	}

	cgi_R_SetColor( otherHUDBits[index].color);

	CG_DrawPic( 
		otherHUDBits[index].xPos,
		otherHUDBits[index].yPos,
		otherHUDBits[index].width, 
		otherHUDBits[index].height, 
		otherHUDBits[index].background 
		);

}

/*
================
CG_DrawAmmo

Draw the ammo graphics (tics) and the ammo numeric amount. Any tics that are partial will
be alphaed out. 
================
*/
static void CG_DrawAmmo(const centity_t	*cent,const int xPos,const int yPos)
{
	playerState_t	*ps;
	int			i;
	vec4_t		calcColor;
	float		currValue=0,inc;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon == WP_STUN_BATON )
	{
		return;
	}

	ps = &cg.snap->ps;

	currValue = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	if (currValue < 0)	// No ammo
	{
		return;
	}


	//
	// ammo
	//
	if (cg.oldammo < currValue)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = currValue;


	// Determine the color of the numeric field

	// Firing or reloading?
	if (( cg.predicted_player_state.weaponstate == WEAPON_FIRING
		&& cg.predicted_player_state.weaponTime > 100 ))
	{
		memcpy(calcColor, colorTable[CT_LTGREY], sizeof(vec4_t));
	} 
	else 
	{
		if ( currValue > 0 ) 
		{
			if (cg.oldAmmoTime > cg.time)
			{
				memcpy(calcColor, colorTable[CT_YELLOW], sizeof(vec4_t));
			}
			else
			{
				memcpy(calcColor, otherHUDBits[OHB_AMMOAMOUNT].color, sizeof(vec4_t));
			}
		} 
		else 
		{
			memcpy(calcColor, colorTable[CT_RED], sizeof(vec4_t));
		}
	}

	// Print number amount
	cgi_R_SetColor( calcColor );	

	CG_DrawNumField (
		otherHUDBits[OHB_AMMOAMOUNT].xPos, 
		otherHUDBits[OHB_AMMOAMOUNT].yPos, 
		3, 
		ps->ammo[weaponData[cent->currentState.weapon].ammoIndex], 
		otherHUDBits[OHB_AMMOAMOUNT].width, 
		otherHUDBits[OHB_AMMOAMOUNT].height, 
		NUM_FONT_SMALL,
		qfalse);



	inc = (float) ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_HUD_TICS;
	currValue =ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			memcpy(calcColor, ammoTics[i].color, sizeof(vec4_t));
			float percent = currValue / inc;
			calcColor[3] *= percent;
		}

		cgi_R_SetColor( calcColor);
		CG_DrawPic( ammoTics[i].xPos, 
			ammoTics[i].yPos, 
			ammoTics[i].width, 
			ammoTics[i].height, 
			ammoTics[i].background );

		currValue -= inc;
	}

}


/*
================
CG_DrawHealth
================
*/
static void CG_DrawHealth(const int x,const int y,const int w,const int h)
{
	vec4_t			calcColor;
	playerState_t	*ps = &cg.snap->ps;

	// Print all the tics of the health graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	float inc = (float) ps->stats[STAT_MAX_HEALTH] / MAX_HUD_TICS;
	float currValue = ps->stats[STAT_HEALTH];
	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
	int i;
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			memcpy(calcColor, healthTics[i].color, sizeof(vec4_t));
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor);

		CG_DrawPic( 
			healthTics[i].xPos,
			healthTics[i].yPos,
			healthTics[i].width, 
			healthTics[i].height, 
			healthTics[i].background 
			);

		currValue -= inc;
	}


	// Print force health amount
	cgi_R_SetColor( otherHUDBits[OHB_HEALTHAMOUNT].color );	

	CG_DrawNumField (
		otherHUDBits[OHB_HEALTHAMOUNT].xPos, 
		otherHUDBits[OHB_HEALTHAMOUNT].yPos, 
		3, 
		ps->stats[STAT_HEALTH], 
		otherHUDBits[OHB_HEALTHAMOUNT].width, 
		otherHUDBits[OHB_HEALTHAMOUNT].height, 
		NUM_FONT_SMALL,
		qfalse);

}

/*
================
CG_DrawArmor

Draw the armor graphics (tics) and the armor numeric amount. Any tics that are partial will
be alphaed out.
================
*/
static void CG_DrawArmor(const int x,const int y,const int w,const int h)
{
	vec4_t			calcColor;
	playerState_t	*ps = &cg.snap->ps;

	// Print all the tics of the armor graphic
	// Look at the amount of armor left and show only as much of the graphic as there is armor.
	// Use alpha to fade out partial section of armor 
	// MAX_HEALTH is the same thing as max armor
	float inc = (float) ps->stats[STAT_MAX_HEALTH] / MAX_HUD_TICS;
	float currValue = ps->stats[STAT_ARMOR];

	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
	int i;
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			memcpy(calcColor, armorTics[i].color, sizeof(vec4_t));
			float percent = currValue / inc;
			calcColor[3] *= percent;
		}

		cgi_R_SetColor( calcColor);

		if ((i==(MAX_HUD_TICS-1)) && (currValue < inc))
		{
			if (cg.HUDArmorFlag)
			{
				CG_DrawPic( 
					armorTics[i].xPos,
					armorTics[i].yPos,
					armorTics[i].width, 
					armorTics[i].height, 
					armorTics[i].background
					);
			}
		}
		else 
		{
			CG_DrawPic( 
				armorTics[i].xPos,
				armorTics[i].yPos,
				armorTics[i].width, 
				armorTics[i].height, 
				armorTics[i].background
				);
		}

		currValue -= inc;
	}

	// Print armor amount
	cgi_R_SetColor( otherHUDBits[OHB_ARMORAMOUNT].color );	

	CG_DrawNumField (
		otherHUDBits[OHB_ARMORAMOUNT].xPos, 
		otherHUDBits[OHB_ARMORAMOUNT].yPos, 
		3, 
		ps->stats[STAT_ARMOR], 
		otherHUDBits[OHB_ARMORAMOUNT].width, 
		otherHUDBits[OHB_ARMORAMOUNT].height, 
		NUM_FONT_SMALL,
		qfalse);


	// If armor is low, flash a graphic to warn the player
	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		float quarterArmor = (float) (ps->stats[STAT_MAX_HEALTH] / 4.0f);

		// Make tic flash if armor is at 25% of full armor
		if (ps->stats[STAT_ARMOR] < quarterArmor)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 400;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

}

#define MAX_VHUD_SHIELD_TICS 12
#define MAX_VHUD_SPEED_TICS 5
#define MAX_VHUD_ARMOR_TICS 5
#define MAX_VHUD_AMMO_TICS 5

static void CG_DrawVehicleSheild( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height,i;
	vec4_t	color,calcColor;
	qhandle_t	background;
	char itemName[64];
	float inc, currValue,maxHealth;

	//riding some kind of living creature 
	if ( pVeh->m_pVehicleInfo->type == VH_ANIMAL || pVeh->m_pVehicleInfo->type == VH_FLIER )
	{
		maxHealth = 100.0f;
		currValue = pVeh->m_pParentEntity->health;
	}
	else //normal vehicle 
	{
		maxHealth = pVeh->m_pVehicleInfo->armor;
		currValue = pVeh->m_iArmor;
	}

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"shieldbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxHealth / MAX_VHUD_SHIELD_TICS;
	for (i=1;i <= MAX_VHUD_SHIELD_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "shield_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor);

		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}

// The HUD.menu file has the graphic print with a negative height, so it will print from the bottom up.
static void CG_DrawVehicleTurboRecharge( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height;
	qhandle_t	background;
	vec4_t	color;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"turborecharge",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		float percent=0.0f;
		int diff = ( cg.time - pVeh->m_iTurboTime );

		// Calc max time

		if (diff > pVeh->m_pVehicleInfo->turboRecharge)
		{
			percent = 1.0f;
			cgi_R_SetColor( colorTable[CT_GREEN] );
		}
		else 
		{
			percent = (float) diff / pVeh->m_pVehicleInfo->turboRecharge;
			if (percent < 0.0f)
			{
				percent = 0.0f;
			}
			cgi_R_SetColor( colorTable[CT_RED] );
		}

		height *= percent;

		CG_DrawPic(xPos,yPos, width, height, cgs.media.whiteShader);	// Top
	}


}

static void CG_DrawVehicleSpeed( const centity_t *cent, const Vehicle_t *pVeh, const char *entHud )
{
	int xPos,yPos,width,height;
	qhandle_t	background;
	gentity_t *parent = pVeh->m_pParentEntity;
	playerState_t *parentPS = &parent->client->ps;
	float currValue,maxSpeed;
	vec4_t	color,calcColor;
	float inc;
	int i;
	char itemName[64];

	if (cgi_UI_GetMenuItemInfo(
		entHud,
		"speedbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	maxSpeed = pVeh->m_pVehicleInfo->speedMax;
	currValue = parentPS->speed;

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxSpeed / MAX_VHUD_SPEED_TICS;
	for (i=1;i<=MAX_VHUD_SPEED_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "speed_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			entHud,
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		if ( level.time > pVeh->m_iTurboTime )
		{
			memcpy(calcColor, color, sizeof(vec4_t));
		}
		else	// In turbo mode
		{
			if (cg.VHUDFlashTime < cg.time)	
			{
				cg.VHUDFlashTime = cg.time + 400;
				if (cg.VHUDTurboFlag)
				{
					cg.VHUDTurboFlag = qfalse;
				}
				else
				{
					cg.VHUDTurboFlag = qtrue;
				}
			}

			if (cg.VHUDTurboFlag)
			{
				memcpy(calcColor, colorTable[CT_LTRED1], sizeof(vec4_t));
			}
			else
			{
				memcpy(calcColor, color, sizeof(vec4_t));
			}
		}


		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor);

		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}

}

static void CG_DrawVehicleArmor( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height,i;
	qhandle_t	background;
	char itemName[64];
	float inc, currValue,maxArmor;
	vec4_t	color,calcColor;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"armorbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	maxArmor = pVeh->m_iArmor;
	currValue = pVeh->m_pVehicleInfo->armor;

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxArmor / MAX_VHUD_ARMOR_TICS;
	for (i=1;i<=MAX_VHUD_ARMOR_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "armor_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor);

		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}

static void CG_DrawVehicleAmmo( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height,i;
	qhandle_t	background;
	char itemName[64];
	float inc, currValue,maxAmmo;
	vec4_t	color,calcColor;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"ammobackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	maxAmmo = pVeh->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = pVeh->weaponStatus[0].ammo;
	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<=MAX_VHUD_AMMO_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "ammo_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor );
		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}

 
static void CG_DrawVehicleAmmoUpper( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height,i;
	qhandle_t	background;
	char itemName[64];
	float inc, currValue,maxAmmo;
	vec4_t	color,calcColor;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"ammoupperbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	maxAmmo = pVeh->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = pVeh->weaponStatus[0].ammo;
	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "ammoupper_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor );
		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}


static void CG_DrawVehicleAmmoLower( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height,i;
	qhandle_t	background;
	char itemName[64];
	float inc, currValue,maxAmmo;
	vec4_t	color,calcColor;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"ammolowerbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	maxAmmo = pVeh->m_pVehicleInfo->weapon[1].ammoMax;
	currValue = pVeh->weaponStatus[1].ammo;
	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "ammolower_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor );
		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}

static void CG_DrawVehicleHud( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height;
	vec4_t	color;
	qhandle_t	background;

	CG_DrawVehicleTurboRecharge( cent, pVeh );

	// Draw frame
	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"leftframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"rightframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}
	
	CG_DrawVehicleSheild( cent, pVeh );

	CG_DrawVehicleSpeed( cent, pVeh, "swoopvehiclehud" );

	CG_DrawVehicleArmor( cent, pVeh );

	CG_DrawVehicleAmmo( cent, pVeh );

	if (0)
	{
		CG_DrawVehicleAmmoUpper( cent, pVeh );
	}

	if (0)
	{
		CG_DrawVehicleAmmoLower( cent, pVeh );
	}

}

static void CG_DrawTauntaunHud( const centity_t *cent, const Vehicle_t *pVeh )
{
	int xPos,yPos,width,height;
	vec4_t	color;
	qhandle_t	background;

	CG_DrawVehicleTurboRecharge( cent, pVeh );

	// Draw frame
	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"leftframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"rightframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}
	
	CG_DrawVehicleSheild( cent, pVeh );

	CG_DrawVehicleSpeed( cent, pVeh, "tauntaunhud" );

	if (0)
	{
		CG_DrawVehicleAmmoUpper( cent, pVeh );
	}

	if (0)
	{
		CG_DrawVehicleAmmoLower( cent, pVeh );
	}

}



static void CG_DrawEmplacedGunHealth( const centity_t *cent )
{
	int xPos,yPos,width,height,i, health=0;
	vec4_t	color,calcColor;
	qhandle_t	background;
	char itemName[64];
	float inc, currValue,maxHealth;

	if ( cent->gent && cent->gent->owner )
	{
		if (( cent->gent->owner->flags & FL_GODMODE ))
		{
			// chair is in godmode, so render the health of the player instead
			health = cent->gent->health;
		}
		else
		{
			// render the chair health
			health = cent->gent->owner->health;
		}
	}
	else
	{
		return;
	}
	//riding some kind of living creature 
	maxHealth = (float)cent->gent->max_health;
	currValue = health;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"shieldbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxHealth / MAX_VHUD_SHIELD_TICS;
	for (i=1;i <= MAX_VHUD_SHIELD_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "shield_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor);

		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}

static void CG_DrawEmplacedGunHud( const centity_t *cent )
{
	int xPos,yPos,width,height;
	vec4_t	color;
	qhandle_t	background;

	// Draw frame
	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"leftframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"rightframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	CG_DrawEmplacedGunHealth( cent );

}


static void CG_DrawItemHealth( float currValue, float maxHealth )
{
	int xPos,yPos,width,height,i;
	vec4_t	color,calcColor;
	qhandle_t	background;
	char itemName[64];
	float inc;

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"shieldbackground",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxHealth / MAX_VHUD_SHIELD_TICS;
	for (i=1;i <= MAX_VHUD_SHIELD_TICS;i++)
	{
		Com_sprintf( itemName, sizeof(itemName), "shield_tic%d",	i );

		if (!cgi_UI_GetMenuItemInfo(
			"swoopvehiclehud",
			itemName,
			&xPos,
			&yPos,
			&width,
			&height,
			color,
			&background))
		{
			continue;
		}

		memcpy(calcColor, color, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		cgi_R_SetColor( calcColor);

		CG_DrawPic( xPos, yPos, width, height, background );

		currValue -= inc;
	}
}

static void CG_DrawPanelTurretHud( void )
{
	int xPos,yPos,width,height;
	vec4_t	color;
	qhandle_t	background;

	// Draw frame
	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"leftframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"rightframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	CG_DrawItemHealth( 
		g_entities[cg.snap->ps.viewEntity].health, 
		(float)g_entities[cg.snap->ps.viewEntity].max_health
		);

}

static void CG_DrawATSTHud( centity_t *cent )
{
	int xPos,yPos,width,height;
	vec4_t	color;
	qhandle_t	background;
	float	health;

	if ( !cg.snap
		||!g_entities[cg.snap->ps.viewEntity].activator )
	{
		return;
	}

	// Draw frame
	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"leftframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"swoopvehiclehud",
		"rightframe",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	// we just calc the display value from the sum of health and armor
	if ( g_entities[cg.snap->ps.viewEntity].activator ) // ensure we can look back to the atst_drivable to get the max health
	{
		health = ( g_entities[cg.snap->ps.viewEntity].health + g_entities[cg.snap->ps.viewEntity].client->ps.stats[STAT_ARMOR] );
	}
	else
	{
		health = ( g_entities[cg.snap->ps.viewEntity].health + g_entities[cg.snap->ps.viewEntity].client->ps.stats[STAT_ARMOR] );
	}

	CG_DrawItemHealth(health,g_entities[cg.snap->ps.viewEntity].activator->max_health );

	if (cgi_UI_GetMenuItemInfo(
		"atsthud",
		"background",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );

		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"atsthud",
		"outer_frame",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, background );
	}

	if (cgi_UI_GetMenuItemInfo(
		"atsthud",
		"left_pic",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );

		CG_DrawPic( xPos, yPos, width, height, background );
	}
}

//-----------------------------------------------------
static qboolean CG_DrawCustomHealthHud( centity_t *cent )
{
	Vehicle_t *pVeh;

	// In a Weapon?
	if (( cent->currentState.eFlags & EF_LOCKED_TO_WEAPON ))
	{
		CG_DrawEmplacedGunHud(cent);

		// DRAW emplaced HUD
/*		color[0] = color[1] = color[2] = 0.0f;
		color[3] = 0.3f;

		cgi_R_SetColor( color );
		CG_DrawPic( 14, 480 - 50, 94, 32, cgs.media.whiteShader );

		// NOTE: this looks ugly
		if ( cent->gent && cent->gent->owner )
		{
			if (( cent->gent->owner->flags & FL_GODMODE ))
			{
				// chair is in godmode, so render the health of the player instead
				health = cent->gent->health / (float)cent->gent->max_health;
			}
			else
			{
				// render the chair health
				health = cent->gent->owner->health / (float)cent->gent->owner->max_health;
			}
		}

		color[0] = 1.0f;
		color[3] = 0.5f;

		cgi_R_SetColor( color );
		CG_DrawPic( 18, 480 - 41, 87 * health, 19, cgs.media.whiteShader );

		cgi_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( 2, 480 - 64, 128, 64, cgs.media.emplacedHealthBarShader);
*/
		return qfalse; // drew this hud, so don't draw the player one
	}
	// In an ATST
	else if (( cent->currentState.eFlags & EF_IN_ATST ))
	{

/*
		// we are an ATST...
		color[0] = color[1] = color[2] = 0.0f;
		color[3] = 0.3f;

		cgi_R_SetColor( color );
		CG_DrawPic( 14, 480 - 50, 94, 32, cgs.media.whiteShader );

		// we just calc the display value from the sum of health and armor
		if ( g_entities[cg.snap->ps.viewEntity].activator ) // ensure we can look back to the atst_drivable to get the max health
		{
			health = ( g_entities[cg.snap->ps.viewEntity].health + g_entities[cg.snap->ps.viewEntity].client->ps.stats[STAT_ARMOR] ) / 
				(float)(g_entities[cg.snap->ps.viewEntity].max_health + g_entities[cg.snap->ps.viewEntity].activator->max_health );
		}
		else
		{
			health = ( g_entities[cg.snap->ps.viewEntity].health + g_entities[cg.snap->ps.viewEntity].client->ps.stats[STAT_ARMOR]) / 
				(float)(g_entities[cg.snap->ps.viewEntity].max_health + 800 ); // hacked max armor since we don't have an activator...should never happen
		}

		color[1] = 0.25f; // blue-green
		color[2] = 1.0f;
		color[3] = 0.5f;

		cgi_R_SetColor( color );
		CG_DrawPic( 18, 480 - 41, 87 * health, 19, cgs.media.whiteShader );

		cgi_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( 2, 480 - 64, 128, 64, cgs.media.emplacedHealthBarShader);
*/
		CG_DrawATSTHud(cent);

		return qfalse; // drew this hud, so don't draw the player one
	}
	// In a vehicle
	else if ( (pVeh = G_IsRidingVehicle( cent->gent ) ) != 0 )
	{

		//riding some kind of living creature 
		if ( pVeh->m_pVehicleInfo->type == VH_ANIMAL )
		{
			CG_DrawTauntaunHud( cent, pVeh );
		}
		else
		{
			CG_DrawVehicleHud( cent, pVeh );
		}
		return qtrue; // draw this hud AND the player one
	}
	// Other?
	else if ( cg.snap->ps.viewEntity && ( g_entities[cg.snap->ps.viewEntity].dflags & DAMAGE_CUSTOM_HUD ))
	{
		CG_DrawPanelTurretHud();
		return qfalse; // drew this hud, so don't draw the player one
	}

	return qtrue;
}

//--------------------------------------
static void CG_DrawBatteryCharge( void )
{
	if ( cg.batteryChargeTime > cg.time )
	{
		vec4_t color;

		// FIXME: drawing it here will overwrite zoom masks...find a better place
		if ( cg.batteryChargeTime < cg.time + 1000 )
		{
			// fading out for the last second
			color[0] = color[1] = color[2] = 1.0f;
			color[3] = (cg.batteryChargeTime - cg.time) / 1000.0f;
		}
		else
		{
			// draw full
			color[0] = color[1] = color[2] = color[3] = 1.0f;
		}

		cgi_R_SetColor( color );

		// batteries were just charged
		CG_DrawPic( 605, 295, 24, 32, cgs.media.batteryChargeShader );
	}
}

#define SimpleHud_DrawString( x, y, str, color ) cgi_R_Font_DrawString( x, y, str, color, (int)0x80000000 | cgs.media.qhFontSmall, -1, 1.0f )

static void CG_DrawSimpleSaberStyle( const centity_t *cent )
{
	uint32_t	calcColor;
	char		num[7] = { 0 };
	int			weapX = 16;

	if ( !cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon != WP_SABER || !cent->gent )
	{
		return;
	}

	if ( !cg.saberAnimLevelPending && cent->gent && cent->gent->client )
	{//uninitialized after a loadgame, cheat across and get it
		cg.saberAnimLevelPending = cent->gent->client->ps.saberAnimLevel;
	}

	switch ( cg.saberAnimLevelPending )
	{
	default:
	case SS_FAST:
		Com_sprintf( num, sizeof( num ), "FAST" );
		calcColor = CT_ICON_BLUE;
		weapX = 0;
		break;
	case SS_MEDIUM:
		Com_sprintf( num, sizeof( num ), "MEDIUM" );
		calcColor = CT_YELLOW;
		break;
	case SS_STRONG:
		Com_sprintf( num, sizeof( num ), "STRONG" );
		calcColor = CT_HUD_RED;
		break;
	case SS_DESANN:
		Com_sprintf( num, sizeof( num ), "DESANN" );
		calcColor = CT_HUD_RED;
		break;
	case SS_TAVION:
		Com_sprintf( num, sizeof( num ), "TAVION" );
		calcColor = CT_ICON_BLUE;
		break;
	case SS_DUAL:
		Com_sprintf( num, sizeof( num ), "AKIMBO" );
		calcColor = CT_HUD_ORANGE;
		break;
	case SS_STAFF:
		Com_sprintf( num, sizeof( num ), "STAFF" );
		calcColor = CT_HUD_ORANGE;
		break;
	}

	SimpleHud_DrawString( SCREEN_WIDTH - (weapX + 16 + 32), (SCREEN_HEIGHT - 80) + 40, num, colorTable[calcColor] );
}

static void CG_DrawSimpleAmmo( const centity_t *cent )
{
	playerState_t	*ps;
	uint32_t	calcColor;
	int			currValue = 0;
	char		num[16] = { 0 };

	if ( !cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	ps = &cg.snap->ps;

	currValue = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	// No ammo
	if ( currValue < 0 || (weaponData[cent->currentState.weapon].energyPerShot == 0 && weaponData[cent->currentState.weapon].altEnergyPerShot == 0) )
	{
		SimpleHud_DrawString( SCREEN_WIDTH - (16 + 32), (SCREEN_HEIGHT - 80) + 40, "--", colorTable[CT_HUD_ORANGE] );
		return;
	}

	//
	// ammo
	//
	if ( cg.oldammo < currValue )
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = currValue;


	// Determine the color of the numeric field

	// Firing or reloading?
	if ( (cg.predicted_player_state.weaponstate == WEAPON_FIRING
		&& cg.predicted_player_state.weaponTime > 100) )
	{
		calcColor = CT_LTGREY;
	}
	else
	{
		if ( currValue > 0 )
		{
			if ( cg.oldAmmoTime > cg.time )
			{
				calcColor = CT_YELLOW;
			}
			else
			{
				calcColor = CT_HUD_ORANGE;
			}
		}
		else
		{
			calcColor = CT_RED;
		}
	}

	Com_sprintf( num, sizeof( num ), "%i", currValue );

	SimpleHud_DrawString( SCREEN_WIDTH - (16 + 32), (SCREEN_HEIGHT - 80) + 40, num, colorTable[calcColor] );
}

static void CG_DrawSimpleForcePower( const centity_t *cent )
{
	uint32_t	calcColor;
	char		num[16] = { 0 };
	qboolean	flash = qfalse;

	if ( !cent->gent || !cent->gent->client->ps.forcePowersKnown )
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if ( cg.forceHUDTotalFlashTime > cg.time )
	{
		flash = qtrue;
		if ( cg.forceHUDNextFlashTime < cg.time )
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			cgi_S_StartSound( NULL, 0, CHAN_AUTO, cgs.media.noforceSound );
			if ( cg.forceHUDActive )
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}

		}
	}
	else	// turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

	// Determine the color of the numeric field
	calcColor = flash ? CT_RED : CT_ICON_BLUE;

	Com_sprintf( num, sizeof( num ), "%i", cent->gent->client->ps.forcePower );

	SimpleHud_DrawString( SCREEN_WIDTH - (16 + 32), (SCREEN_HEIGHT - 80) + 40 + 14, num, colorTable[calcColor] );
}

/*
================
CG_DrawHUD
================
*/
static void CG_DrawHUD( centity_t *cent )
{
	int value;
	int	sectionXPos,sectionYPos,sectionWidth,sectionHeight;

	if ( cg_hudFiles.integer )
	{
		int x = 0;
		int y = SCREEN_HEIGHT - 80;

		SimpleHud_DrawString( x + 16, y + 40, va( "%i", cg.snap->ps.stats[STAT_HEALTH] ), colorTable[CT_HUD_RED] );

		SimpleHud_DrawString( x + 18 + 14, y + 40 + 14, va( "%i", cg.snap->ps.stats[STAT_ARMOR] ), colorTable[CT_HUD_GREEN] );

		CG_DrawSimpleForcePower( cent );

		if ( cent->currentState.weapon == WP_SABER )
			CG_DrawSimpleSaberStyle( cent );
		else
			CG_DrawSimpleAmmo( cent );

		return;
	}

	// Draw the lower left section of the HUD
	if (cgi_UI_GetMenuInfo("lefthud",&sectionXPos,&sectionYPos,&sectionWidth,&sectionHeight))
	{
		// Draw all the HUD elements --eez
		cgi_UI_Menu_Paint( cgi_UI_GetMenuByName( "lefthud" ), qtrue );

		// Draw armor & health values
		if ( cg_drawStatus.integer == 2 ) 
		{
			CG_DrawSmallStringColor(sectionXPos+5, sectionYPos - 60,va("Armor:%d",cg.snap->ps.stats[STAT_ARMOR]), colorTable[CT_HUD_GREEN] );
			CG_DrawSmallStringColor(sectionXPos+5, sectionYPos - 40,va("Health:%d",cg.snap->ps.stats[STAT_HEALTH]), colorTable[CT_HUD_GREEN] );
		}

		// Print scanline
		cgi_R_SetColor( otherHUDBits[OHB_SCANLINE_LEFT].color);

		CG_DrawPic( 
			otherHUDBits[OHB_SCANLINE_LEFT].xPos,
			otherHUDBits[OHB_SCANLINE_LEFT].yPos,
			otherHUDBits[OHB_SCANLINE_LEFT].width, 
			otherHUDBits[OHB_SCANLINE_LEFT].height, 
			otherHUDBits[OHB_SCANLINE_LEFT].background 
			);

		// Print frame
		cgi_R_SetColor( otherHUDBits[OHB_FRAME_LEFT].color);
		CG_DrawPic( 
			otherHUDBits[OHB_FRAME_LEFT].xPos,
			otherHUDBits[OHB_FRAME_LEFT].yPos,
			otherHUDBits[OHB_FRAME_LEFT].width, 
			otherHUDBits[OHB_FRAME_LEFT].height, 
			otherHUDBits[OHB_FRAME_LEFT].background 
			);

		CG_DrawArmor(sectionXPos,sectionYPos,sectionWidth,sectionHeight);

		CG_DrawHealth(sectionXPos,sectionYPos,sectionWidth,sectionHeight);
	}


	// Draw the lower right section of the HUD
	if (cgi_UI_GetMenuInfo("righthud",&sectionXPos,&sectionYPos,&sectionWidth,&sectionHeight))
	{
		// Draw all the HUD elements --eez
		cgi_UI_Menu_Paint( cgi_UI_GetMenuByName( "righthud" ), qtrue );

		// Draw armor & health values
		if ( cg_drawStatus.integer == 2 ) 
		{
			if ( cent->currentState.weapon != WP_SABER && cent->currentState.weapon != WP_STUN_BATON && cent->gent )
			{
				value = cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex];
				CG_DrawSmallStringColor(sectionXPos, sectionYPos - 60,va("Ammo:%d",value), colorTable[CT_HUD_GREEN] );
			}
			CG_DrawSmallStringColor(sectionXPos, sectionYPos - 40,va("Force:%d",cent->gent->client->ps.forcePower), colorTable[CT_HUD_GREEN] );
		}

		// Print scanline
		cgi_R_SetColor( otherHUDBits[OHB_SCANLINE_RIGHT].color);

		CG_DrawPic( 
			otherHUDBits[OHB_SCANLINE_RIGHT].xPos,
			otherHUDBits[OHB_SCANLINE_RIGHT].yPos,
			otherHUDBits[OHB_SCANLINE_RIGHT].width, 
			otherHUDBits[OHB_SCANLINE_RIGHT].height, 
			otherHUDBits[OHB_SCANLINE_RIGHT].background 
			);


		// Print frame
		cgi_R_SetColor( otherHUDBits[OHB_FRAME_RIGHT].color);
		CG_DrawPic( 
			otherHUDBits[OHB_FRAME_RIGHT].xPos,
			otherHUDBits[OHB_FRAME_RIGHT].yPos,
			otherHUDBits[OHB_FRAME_RIGHT].width, 
			otherHUDBits[OHB_FRAME_RIGHT].height, 
			otherHUDBits[OHB_FRAME_RIGHT].background 
			);

		CG_DrawForcePower(cent,sectionXPos,sectionYPos);

		// Draw ammo tics or saber style
		if ( cent->currentState.weapon == WP_SABER )
		{
			CG_DrawSaberStyle(cent,sectionXPos,sectionYPos);
		}
		else
		{
			CG_DrawAmmo(cent,sectionXPos,sectionYPos);
		}
//		CG_DrawMessageLit(cent,x,y);
	}
}

/*
================
CG_ClearDataPadCvars
================
*/
void CG_ClearDataPadCvars( void )
{
	cgi_Cvar_Set( "cg_updatedDataPadForcePower1", "0" );
	cgi_Cvar_Update( &cg_updatedDataPadForcePower1 );
	cgi_Cvar_Set( "cg_updatedDataPadForcePower2", "0" );
	cgi_Cvar_Update( &cg_updatedDataPadForcePower2 );
	cgi_Cvar_Set( "cg_updatedDataPadForcePower3", "0" );
	cgi_Cvar_Update( &cg_updatedDataPadForcePower3 );

	cgi_Cvar_Set( "cg_updatedDataPadObjective", "0" );
	cgi_Cvar_Update( &cg_updatedDataPadObjective );
}

/*
================
CG_DrawDataPadHUD
================
*/
void CG_DrawDataPadHUD( centity_t *cent )
{
	int x,y;

	x = 34;
	y = 286;

	CG_DrawHealth(x,y,80,80);

	x = 526;

	if ((missionInfo_Updated) && ((cg_updatedDataPadForcePower1.integer) || (cg_updatedDataPadObjective.integer)))
	{
		// Stop flashing light
		cg.missionInfoFlashTime = 0;
		missionInfo_Updated = qfalse;

		// Set which force power to show.
		// cg_updatedDataPadForcePower is set from Q3_Interface, because force powers would only be given 
		// from a script.
		if (cg_updatedDataPadForcePower1.integer)
		{
			cg.DataPadforcepowerSelect = cg_updatedDataPadForcePower1.integer - 1; // Not pretty, I know
			if (cg.DataPadforcepowerSelect >= MAX_DPSHOWPOWERS)
			{	//duh
				cg.DataPadforcepowerSelect = MAX_DPSHOWPOWERS-1;
			}
			else if (cg.DataPadforcepowerSelect<0)
			{
				cg.DataPadforcepowerSelect=0;
			}
		}
//		CG_ClearDataPadCvars();
	}

	CG_DrawForcePower(cent,x,y);
	CG_DrawAmmo(cent,x,y);
	CG_DrawMessageLit(cent,x,y);

	cgi_R_SetColor( colorTable[CT_WHITE]);
	CG_DrawPic( 0, 0, 640, 480, cgs.media.dataPadFrame );

}

//------------------------
// CG_DrawZoomMask
//------------------------
static void CG_DrawBinocularNumbers( qboolean power )
{
	vec4_t color1;

	cgi_R_SetColor( colorTable[CT_BLACK]);
	CG_DrawPic( 212, 367, 200, 40, cgs.media.whiteShader );

	if ( power )
	{
		// Numbers should be kind of greenish
		color1[0] = 0.2f;
		color1[1] = 0.4f;
		color1[2] = 0.2f;
		color1[3] = 0.3f;
		cgi_R_SetColor( color1 );

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		int val = ((int)((cg.refdefViewAngles[YAW] + 180) / 10)) * 10;
		float off = (cg.refdefViewAngles[YAW] + 180) - val;

		for ( int i = -10; i < 30; i += 10 )
		{
			val -= 10;

			if ( val < 0 )
			{
				val += 360;
			}

			// we only want to draw the very far left one some of the time, if it's too far to the left it will poke outside the mask.
			if (( off > 3.0f && i == -10 ) || i > -10 )
			{
				// draw the value, but add 200 just to bump the range up...arbitrary, so change it if you like
				CG_DrawNumField( 155 + i * 10 + off * 10, 374, 3, val + 200, 24, 14, NUM_FONT_CHUNKY, qtrue );
				CG_DrawPic( 245 + (i-1) * 10 + off * 10, 376, 6, 6, cgs.media.whiteShader );
			}
		}

		CG_DrawPic( 212, 367, 200, 28, cgs.media.binocularOverlay );
	}
}

/*
================
CG_DrawZoomMask

================
*/
extern float cg_zoomFov;	//from cg_view.cpp

static void CG_DrawZoomMask( void )
{
	vec4_t			color1;
	centity_t		*cent;
	float			level;
	static qboolean	flip = qtrue;
	float			charge = cg.snap->ps.batteryCharge / (float)MAX_BATTERIES; // convert charge to a percentage
	qboolean		power = qfalse;

	cent = &cg_entities[0];

	if ( charge > 0.0f )
	{
		power = qtrue;
	}

	//-------------
	// Binoculars
	//--------------------------------
	if ( cg.zoomMode == 1 )
	{
		CG_RegisterItemVisuals( ITM_BINOCULARS_PICKUP );

		// zoom level
		level = (float)(80.0f - cg_zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		if ( power )
		{
			// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
			cgi_R_SetColor( colorTable[CT_WHITE] );
			CG_DrawPic( 34, 48, 570, 362, cgs.media.binocularStatic );
		}
	
		CG_DrawBinocularNumbers( power );

		// Black out the area behind the battery display
		cgi_R_SetColor( colorTable[CT_DKGREY]);
		CG_DrawPic( 50, 389, 161, 16, cgs.media.whiteShader );

		if ( power )
		{
			color1[0] = sin( cg.time * 0.01f ) * 0.5f + 0.5f;
			color1[0] = color1[0] * color1[0];
			color1[1] = color1[0];
			color1[2] = color1[0];
			color1[3] = 1.0f;

			cgi_R_SetColor( color1 );

			CG_DrawPic( 82, 94, 16, 16, cgs.media.binocularCircle );
		}

		CG_DrawPic( 0, 0, 640, 480, cgs.media.binocularMask );

		if ( power )
		{
			// Flickery color
			color1[0] = 0.7f + crandom() * 0.1f;
			color1[1] = 0.8f + crandom() * 0.1f;
			color1[2] = 0.7f + crandom() * 0.1f;
			color1[3] = 1.0f;
			cgi_R_SetColor( color1 );
		
			CG_DrawPic( 4, 282 - level, 16, 16, cgs.media.binocularArrow );
		}
		else
		{
			// No power color
			color1[0] = 0.15f;
			color1[1] = 0.15f;
			color1[2] = 0.15f;
			color1[3] = 1.0f;
			cgi_R_SetColor( color1 );
		}

		// The top triangle bit randomly flips when the power is on
		if ( flip && power )
		{
			CG_DrawPic( 330, 60, -26, -30, cgs.media.binocularTri );
		}
		else
		{
			CG_DrawPic( 307, 40, 26, 30, cgs.media.binocularTri );
		}

		if ( random() > 0.98f && ( cg.time & 1024 ))
		{
			flip = !flip;
		}

		if ( power )
		{
			color1[0] = 1.0f * ( charge < 0.2f ? !!(cg.time & 256) : 1 );
			color1[1] = charge * color1[0];
			color1[2] = 0.0f;
			color1[3] = 0.2f;

			cgi_R_SetColor( color1 );
			CG_DrawPic( 60, 394.5f, charge * 141, 5, cgs.media.whiteShader );
		}
	}
	//------------
	// Disruptor 
	//--------------------------------
	else if ( cg.zoomMode == 2 )
	{
		level = (float)(80.0f - cg_zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork. 
		level *= 103.0f;

		// Draw target mask
		cgi_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( 0, 0, 640, 480, cgs.media.disruptorMask );

		// apparently 99.0f is the full zoom level
		if ( level >= 99 )
		{
			// Fully zoomed, so make the rotating insert pulse
			color1[0] = 1.0f; 
			color1[1] = 1.0f;
			color1[2] = 1.0f;
			color1[3] = 0.7f + sin( cg.time * 0.01f ) * 0.3f;

			cgi_R_SetColor( color1 );
		}

		// Draw rotating insert
		CG_DrawRotatePic2( 320, 240, 640, 480, -level, cgs.media.disruptorInsert );

		float cx, cy;
		float max;

		max = cg_entities[0].gent->client->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / (float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max;

		if ( max > 1.0f )
		{
			max = 1.0f;
		}

		color1[0] = (1.0f - max) * 2.0f; 
		color1[1] = max * 1.5f;
		color1[2] = 0.0f;
		color1[3] = 1.0f;

		// If we are low on ammo, make us flash
		if ( max < 0.15f && ( cg.time & 512 ))
		{
			VectorClear( color1 );
		}

		if ( color1[0] > 1.0f )
		{
			color1[0] = 1.0f;
		}

		if ( color1[1] > 1.0f )
		{
			color1[1] = 1.0f;
		}

		cgi_R_SetColor( color1 );

		max *= 58.0f;

		for ( float i = 18.5f; i <= 18.5f + max; i+= 3 ) // going from 15 to 45 degrees, with 5 degree increments
		{
			cx = 320 + sin( (i+90.0f)/57.296f ) * 190;
			cy = 240 + cos( (i+90.0f)/57.296f ) * 190;

			CG_DrawRotatePic2( cx, cy, 12, 24, 90 - i, cgs.media.disruptorInsertTick );
		}

		// FIXME: doesn't know about ammo!! which is bad because it draws charge beyond what ammo you may have..
		if ( cg_entities[0].gent->client->ps.weaponstate == WEAPON_CHARGING_ALT )
		{
			cgi_R_SetColor( colorTable[CT_WHITE] );

			// draw the charge level
			max = ( cg.time - cg_entities[0].gent->client->ps.weaponChargeTime ) / ( 150.0f * 10.0f ); // bad hardcodedness 150 is disruptor charge unit and 10 is max charge units allowed.

			if ( max > 1.0f )
			{
				max = 1.0f;
			}

			CG_DrawPic2( 257, 435, 134 * max, 34, 0,0,max,1,cgi_R_RegisterShaderNoMip( "gfx/2d/crop_charge" ));
		}
	}
	//-----------
	// Light Amp
	//--------------------------------
	else if ( cg.zoomMode == 3 )
	{
		CG_RegisterItemVisuals( ITM_LA_GOGGLES_PICKUP );

		if ( power )
		{
			cgi_R_SetColor( colorTable[CT_WHITE] );
			CG_DrawPic( 34, 29, 580, 410, cgs.media.laGogglesStatic );

			CG_DrawPic( 570, 140, 12, 160, cgs.media.laGogglesSideBit );
			
			float light = (128-cent->gent->lightLevel) * 0.5f;

			if ( light < -81 ) // saber can really jack up local light levels....?magic number??
			{
				light = -81;
			}

			float pos1 = 220 + light;
			float pos2 = 220 + cos( cg.time * 0.0004f + light * 0.05f ) * 40 + sin( cg.time * 0.0013f + 1 ) * 20 + sin( cg.time * 0.0021f ) * 5;

			// Flickery color
			color1[0] = 0.7f + crandom() * 0.2f;
			color1[1] = 0.8f + crandom() * 0.2f;
			color1[2] = 0.7f + crandom() * 0.2f;
			color1[3] = 1.0f;
			cgi_R_SetColor( color1 );

			CG_DrawPic( 565, pos1, 22, 8, cgs.media.laGogglesBracket );
			CG_DrawPic( 558, pos2, 14, 5, cgs.media.laGogglesArrow );
		}

		// Black out the area behind the battery display
		cgi_R_SetColor( colorTable[CT_DKGREY]);
		CG_DrawPic( 236, 357, 164, 16, cgs.media.whiteShader );

		if ( power )
		{
			// Power bar
			color1[0] = 1.0f * ( charge < 0.2f ? !!(cg.time & 256) : 1 );
			color1[1] = charge * color1[0];
			color1[2] = 0.0f;
			color1[3] = 0.4f;

			cgi_R_SetColor( color1 );
			CG_DrawPic( 247.0f, 362.5f, charge * 143.0f, 6, cgs.media.whiteShader );

			// pulsing dot bit
			color1[0] = sin( cg.time * 0.01f ) * 0.5f + 0.5f;
			color1[0] = color1[0] * color1[0];
			color1[1] = color1[0];
			color1[2] = color1[0];
			color1[3] = 1.0f;

			cgi_R_SetColor( color1 );

			CG_DrawPic( 65, 94, 16, 16, cgs.media.binocularCircle );
		}
	
		CG_DrawPic( 0, 0, 640, 480, cgs.media.laGogglesMask );
	}
}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats( void ) 
{
	centity_t		*cent;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ((cg.snap->ps.viewEntity>0&&cg.snap->ps.viewEntity<ENTITYNUM_WORLD))
	{
		// MIGHT try and draw a custom hud if it wants...
		CG_DrawCustomHealthHud( cent );
		return;
	}

	cgi_UI_MenuPaintAll();

	qboolean drawHud = qtrue;

	if ( cent && cent->gent )
	{
		drawHud = CG_DrawCustomHealthHud( cent );
	}

	if (( drawHud ) && ( cg_drawHUD.integer ))
	{
		CG_DrawHUD( cent );	
	}
}


/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int		value;
	float	*fadeColor;

	value = cg.itemPickup;
	if ( value && cg_items[ value ].icon != -1 ) 
	{
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) 
		{
			CG_RegisterItemVisuals( value );
			cgi_R_SetColor( fadeColor );
			CG_DrawPic( 573, 320, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			//CG_DrawBigString( ICON_SIZE + 16, 398, bg_itemlist[ value ].classname, fadeColor[0] );
			//CG_DrawProportionalString( ICON_SIZE + 16, 398, 
			//	bg_itemlist[ value ].classname, CG_SMALLFONT,fadeColor );
			cgi_R_SetColor( NULL );
		}
	}
}

void CMD_CGCam_Disable( void );

/*
===================
CG_DrawPickupItem
===================
*/
void CG_DrawCredits(void)
{
	if (!cg.creditsStart)
	{
		//
		cg.creditsStart = qtrue;
		CG_Credits_Init("CREDITS_RAVEN", &colorTable[CT_ICON_BLUE]);
		if ( cg_skippingcin.integer )
		{//Were skipping a cinematic and it's over now
			gi.cvar_set("timescale", "1");
			gi.cvar_set("skippingCinematic", "0");
		}
	}


	if (cg.creditsStart)
	{
		if ( !CG_Credits_Running() ) 
		{
			cgi_Cvar_Set( "cg_endcredits", "0" );
			CMD_CGCam_Disable();
			cgi_SendConsoleCommand("disconnect\n");
		}
	}
}

//draw the health bar based on current "health" and maxhealth
void CG_DrawHealthBar(centity_t *cent, float chX, float chY, float chW, float chH)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = chX-(chW/2);
	float y = chY-chH;
	float percent = 0.0f;
	
	if ( !cent || !cent->gent )
	{
		return;
	}
	percent = ((float)cent->gent->health/(float)cent->gent->max_health);
	if (percent <= 0)
	{
		return;
	}

	//color of the bar
	//hostile
	aColor[0] = 1.0f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, chW, chH, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, (percent*chW)-1.0f, chH-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+(percent*chW), y+1.0f, chW-(percent*chW)-1.0f, chH-1.0f, cColor);
}

#define MAX_HEALTH_BAR_ENTS 32
int cg_numHealthBarEnts = 0;
int cg_healthBarEnts[MAX_HEALTH_BAR_ENTS];
#define HEALTH_BAR_WIDTH	50
#define HEALTH_BAR_HEIGHT	5

void CG_DrawHealthBars( void )
{
	float chX=0, chY=0;
	centity_t *cent;
	vec3_t pos;
	for ( int i = 0; i < cg_numHealthBarEnts; i++ )
	{
		cent = &cg_entities[cg_healthBarEnts[i]];
		if ( cent && cent->gent )
		{
			VectorCopy( cent->lerpOrigin, pos );
			pos[2] += cent->gent->maxs[2]+HEALTH_BAR_HEIGHT+8;
			if ( CG_WorldCoordToScreenCoordFloat( pos, &chX, &chY ) )
			{//on screen
				CG_DrawHealthBar( cent, chX, chY, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT );
			}
		}
	}
}

#define HEALTHBARRANGE 422
void CG_AddHealthBarEnt( int entNum )
{
	if ( cg_numHealthBarEnts >= MAX_HEALTH_BAR_ENTS )
	{//FIXME: Debug error message?
		return;
	}

	if (DistanceSquared( cg_entities[entNum].lerpOrigin, g_entities[0].client->renderInfo.eyePoint ) < (HEALTHBARRANGE*HEALTHBARRANGE) )
	{
		cg_healthBarEnts[cg_numHealthBarEnts++] = entNum;
	}
}

void CG_ClearHealthBarEnts( void )
{
	if ( cg_numHealthBarEnts )
	{
		cg_numHealthBarEnts = 0;
		memset( &cg_healthBarEnts, 0, MAX_HEALTH_BAR_ENTS );
	}
}
/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( vec3_t worldPoint ) 
{
	float		w, h;
	qhandle_t	hShader;
	qboolean	corona = qfalse;
	vec4_t		ecolor;
	float		f;
	float		x, y;

	if ( !cg_drawCrosshair.integer ) 
	{
		return;
	}

	if ( cg.zoomMode > 0 && cg.zoomMode < 3 )
	{
		//not while scoped
		return;
	}

	//set color based on what kind of ent is under crosshair
	if ( g_crosshairEntNum >= ENTITYNUM_WORLD )
	{
		ecolor[0] = ecolor[1] = ecolor[2] = 1.0f;
	}
	else if ( cg_forceCrosshair && cg_crosshairForceHint.integer )
	{
		ecolor[0] = 0.2f;
		ecolor[1] = 0.5f;
		ecolor[2] = 1.0f;

		corona = qtrue;
	}
	else if ( cg_crosshairIdentifyTarget.integer )
	{
		gentity_t *crossEnt = &g_entities[g_crosshairEntNum];

		if ( crossEnt->client )
		{
			if ( crossEnt->client->ps.powerups[PW_CLOAKED] )
			{//cloaked don't show up
				ecolor[0] = 1.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 1.0f;//B
			}
			else if ( g_entities[0].client && g_entities[0].client->playerTeam == TEAM_FREE )
			{//evil player: everyone is red
				//Enemies are red
				ecolor[0] = 1.0f;//R
				ecolor[1] = 0.1f;//G
				ecolor[2] = 0.1f;//B
			}
			else if ( crossEnt->client->playerTeam == TEAM_PLAYER )
			{
				//Allies are green
				ecolor[0] = 0.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 0.0f;//B
			}
			else if ( crossEnt->client->playerTeam == TEAM_NEUTRAL )
			{
				// NOTE: was yellow, but making it white unless they really decide they want to see colors
				ecolor[0] = 1.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 1.0f;//B
			}
			else
			{
				//Enemies are red
				ecolor[0] = 1.0f;//R
				ecolor[1] = 0.1f;//G
				ecolor[2] = 0.1f;//B
			}
		}
		else if ( crossEnt->s.weapon == WP_TURRET && (crossEnt->svFlags&SVF_NONNPC_ENEMY) )
		{
			// a turret
			if ( crossEnt->noDamageTeam == TEAM_PLAYER )
			{
				// mine are green
				ecolor[0] = 0.0;//R
				ecolor[1] = 1.0;//G
				ecolor[2] = 0.0;//B
			}
			else
			{
				// hostile ones are red
				ecolor[0] = 1.0;//R
				ecolor[1] = 0.0;//G
				ecolor[2] = 0.0;//B
			}
		}
		else if ( crossEnt->s.weapon == WP_TRIP_MINE )
		{
			// tripmines are red
			ecolor[0] = 1.0;//R
			ecolor[1] = 0.0;//G
			ecolor[2] = 0.0;//B
		}
		else if ( (crossEnt->flags&FL_RED_CROSSHAIR) )
		{//special case flagged to turn crosshair red
			ecolor[0] = 1.0;//R
			ecolor[1] = 0.0;//G
			ecolor[2] = 0.0;//B
		}
		else
		{
			VectorCopy( crossEnt->startRGBA, ecolor );

			if ( !ecolor[0] && !ecolor[1] && !ecolor[2] )
			{
				// We don't want a black crosshair, so use white since it will show up better
				ecolor[0] = 1.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 1.0f;//B
			}
		}
	}
	else // cg_crosshairIdentifyTarget is not on, so make it white
	{
		ecolor[0] = ecolor[1] = ecolor[2] = 1.0f;
	}

	ecolor[3] = 1.0;
	cgi_R_SetColor( ecolor );

	if ( cg.forceCrosshairStartTime )
	{
		// both of these calcs will fade the corona in one direction
		if ( cg.forceCrosshairEndTime )
		{
			ecolor[3] = (cg.time - cg.forceCrosshairEndTime) / 500.0f;
		}
		else
		{
			ecolor[3] = (cg.time - cg.forceCrosshairStartTime) / 300.0f;
		}

		// clamp 
		if ( ecolor[3] < 0 )
		{
			ecolor[3] = 0;
		}
		else if ( ecolor[3] > 1.0f )
		{
			ecolor[3] = 1.0f;
		}

		if ( !cg.forceCrosshairEndTime )
		{
			// but for the other direction, we'll need to reverse it
			ecolor[3] = 1.0f - ecolor[3];
		}
	}

	if ( corona ) // we are pointing at a crosshair item
	{
		if ( !cg.forceCrosshairStartTime )
		{
			// must have just happened because we are not fading in yet...start it now
			cg.forceCrosshairStartTime = cg.time;
			cg.forceCrosshairEndTime = 0;
		}
		if ( cg.forceCrosshairEndTime )
		{
			// must have just gone over a force thing again...and we were in the process of fading out.  Set fade in level to the level where the fade left off
			cg.forceCrosshairStartTime = cg.time - ( 1.0f - ecolor[3] ) * 300.0f;
			cg.forceCrosshairEndTime = 0;
		}
	}
	else // not pointing at a crosshair item
	{
		if ( cg.forceCrosshairStartTime && !cg.forceCrosshairEndTime ) // were currently fading in
		{
			// must fade back out, but we will have to set the fadeout time to be equal to the current level of faded-in-edness
			cg.forceCrosshairEndTime = cg.time - ecolor[3] * 500.0f;
		}
		if ( cg.forceCrosshairEndTime && cg.time - cg.forceCrosshairEndTime > 500.0f ) // not pointing at anything and fade out is totally done
		{
			// reset everything
			cg.forceCrosshairStartTime = 0;
			cg.forceCrosshairEndTime = 0;
		}
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	if ( worldPoint && VectorLength( worldPoint ) )
	{
		if ( !CG_WorldCoordToScreenCoordFloat( worldPoint, &x, &y ) )
		{//off screen, don't draw it
			cgi_R_SetColor( NULL );
			return;
		}
		x -= 320;//????
		y -= 240;//????
	}
	else
	{
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
	}

	if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{
		if ( !Q_stricmp( "misc_panel_turret", g_entities[cg.snap->ps.viewEntity].classname ))
		{
			// draws a custom crosshair that is twice as large as normal
			cgi_R_DrawStretchPic( x + cg.refdef.x + 320 - w, 
				y + cg.refdef.y + 240 - h, 
				w * 2, h * 2, 0, 0, 1, 1, cgs.media.turretCrossHairShader );	

		}
	}
	else 
	{
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];

		cgi_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (640 - w), 
			y + cg.refdef.y + 0.5 * (480 - h), 
			w, h, 0, 0, 1, 1, hShader );	
	}

	if ( cg.forceCrosshairStartTime && cg_crosshairForceHint.integer ) // drawing extra bits
	{
		ecolor[0] = ecolor[1] = ecolor[2] = (1 - ecolor[3]) * ( sin( cg.time * 0.001f ) * 0.08f + 0.35f ); // don't draw full color
		ecolor[3] = 1.0f;

		cgi_R_SetColor( ecolor );

		w *= 2.0f;
		h *= 2.0f;

		cgi_R_DrawStretchPic( x + cg.refdef.x + 0.5f * ( 640 - w ), y + cg.refdef.y + 0.5f * ( 480 - h ), 
								w, h, 
								0, 0, 1, 1, 
								cgs.media.forceCoronaShader ); 
	}

	cgi_R_SetColor( NULL );
}

/*
qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, int *x, int *y)

  Take any world coord and convert it to a 2D virtual 640x480 screen coord
*/
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y)
{
    vec3_t trans;
    float xc, yc;
    float px, py;
    float z;

    px = tan(cg.refdef.fov_x * (M_PI / 360) );
    py = tan(cg.refdef.fov_y * (M_PI / 360) );

    VectorSubtract(worldCoord, cg.refdef.vieworg, trans);

    xc = 640 / 2.0;
    yc = 480 / 2.0;

	// z = how far is the object in our forward direction
    z = DotProduct(trans, cg.refdef.viewaxis[0]);
    if (z <= 0.001)
        return qfalse;

    *x = xc - DotProduct(trans, cg.refdef.viewaxis[1])*xc/(z*px);
    *y = yc - DotProduct(trans, cg.refdef.viewaxis[2])*yc/(z*py);

    return qtrue;
}

qboolean CG_WorldCoordToScreenCoord( vec3_t worldCoord, int *x, int *y ) {
	float xF, yF;

	if ( CG_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF ) ) {
		*x = (int)xF;
		*y = (int)yF;
		return qtrue;
	}

	return qfalse;
}

// I'm keeping the rocket tracking code separate for now since I may want to do different logic...but it still uses trace info from scanCrosshairEnt
//-----------------------------------------
static void CG_ScanForRocketLock( void )
//-----------------------------------------
{
	gentity_t	*traceEnt;
	static qboolean tempLock = qfalse; // this will break if anything else uses this locking code ( other than the player )

	traceEnt = &g_entities[g_crosshairEntNum];

	if ( !traceEnt || g_crosshairEntNum <= 0 || g_crosshairEntNum >= ENTITYNUM_WORLD || (!traceEnt->client && traceEnt->s.weapon != WP_TURRET ) || !traceEnt->health 
			|| ( traceEnt && traceEnt->client && traceEnt->client->ps.powerups[PW_CLOAKED] ))
	{
		// see how much locking we have
		int dif = ( cg.time - g_rocketLockTime ) / ( 1200.0f / 8.0f );

		// 8 is full locking....also if we just traced onto the world, 
		//	give them 1/2 second of slop before dropping the lock
		if ( dif < 8 && g_rocketSlackTime + 500 < cg.time )
		{
			// didn't have a full lock and not in grace period, so drop the lock
			g_rocketLockTime = 0;
			g_rocketSlackTime = 0;
			tempLock = qfalse;
		}

		if ( g_rocketSlackTime + 500 >= cg.time && g_rocketLockEntNum < ENTITYNUM_WORLD )
		{
			// were locked onto an ent, aren't right now.....but still within the slop grace period
			//	keep the current lock amount
			g_rocketLockTime += cg.frametime;
		}

		if ( !tempLock && g_rocketLockEntNum < ENTITYNUM_WORLD && dif >= 8 )
		{
			tempLock = qtrue;

			if ( g_rocketLockTime + 1200 < cg.time )
			{
				g_rocketLockTime = cg.time - 1200; // doh, hacking the time so the targetting still gets drawn full
			}
		} 		

		// keep locking to this thing for one second after it gets out of view
		if ( g_rocketLockTime + 2000.0f < cg.time ) // since time was hacked above, I'm compensating so that 2000ms is really only 1000ms
		{
			// too bad, you had your chance
			g_rocketLockEntNum = ENTITYNUM_NONE;
			g_rocketSlackTime = 0;
			g_rocketLockTime = 0;
		}
	}
	else
	{
		tempLock = qfalse;

		if ( g_rocketLockEntNum >= ENTITYNUM_WORLD )
		{
			if ( g_rocketSlackTime + 500 < cg.time )
			{
				// we just locked onto something, start the lock at the current time
				g_rocketLockEntNum = g_crosshairEntNum;
				g_rocketLockTime = cg.time;
				g_rocketSlackTime = cg.time;
			}
		}
		else
		{
			if ( g_rocketLockEntNum != g_crosshairEntNum )
			{
				g_rocketLockTime = cg.time;
			}

			// may as well always set this when we can potentially lock to something
			g_rocketSlackTime = cg.time;
			g_rocketLockEntNum = g_crosshairEntNum;
		}
	}
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
extern float forcePushPullRadius[];
static void CG_ScanForCrosshairEntity( qboolean scanAll ) 
{
	trace_t		trace;
	gentity_t	*traceEnt = NULL;
	vec3_t		start, end;
	int			content;
	int			ignoreEnt = cg.snap->ps.clientNum;
	Vehicle_t *pVeh = NULL;

	//FIXME: debounce this to about 10fps?

	cg_forceCrosshair = qfalse;
	if ( cg_entities[0].gent && cg_entities[0].gent->client ) // <-Mike said it should always do this   //if (cg_crosshairForceHint.integer &&
	{//try to check for force-affectable stuff first
		vec3_t d_f, d_rt, d_up;

		// If you're riding a vehicle and not being drawn.
		if ( ( pVeh = G_IsRidingVehicle( cg_entities[0].gent ) ) != NULL && cg_entities[0].currentState.eFlags & EF_NODRAW )
		{
			VectorCopy( cg_entities[pVeh->m_pParentEntity->s.number].lerpOrigin, start );
			AngleVectors( cg_entities[pVeh->m_pParentEntity->s.number].lerpAngles, d_f, d_rt, d_up );
		}
		else
		{
			VectorCopy( g_entities[0].client->renderInfo.eyePoint, start );
			AngleVectors( cg_entities[0].lerpAngles, d_f, d_rt, d_up );
		}

		VectorMA( start, 2048, d_f, end );//4028 is max for mind trick

		//YES!  This is very very bad... but it works!  James made me do it.  Really, he did.  Blame James.
		gi.trace( &trace, start, vec3_origin, vec3_origin, end, 
			ignoreEnt, MASK_OPAQUE|CONTENTS_SHOTCLIP|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_TERRAIN, G2_NOCOLLIDE, 10 );// ); took out CONTENTS_SOLID| so you can target people through glass.... took out CONTENTS_CORPSE so disintegrated guys aren't shown, could just remove their body earlier too...

		if ( trace.entityNum < ENTITYNUM_WORLD )
		{//hit something
			traceEnt = &g_entities[trace.entityNum];
			if ( traceEnt )
			{
				// Check for mind trickable-guys
				if ( traceEnt->client )
				{//is a client
					if ( cg_entities[0].gent->client->ps.forcePowerLevel[FP_TELEPATHY] && traceEnt->health > 0 && VALIDSTRING(traceEnt->behaviorSet[BSET_MINDTRICK]) )
					{//I have the ability to mind-trick and he is alive and he has a mind trick script
						//NOTE: no need to check range since it's always 2048
						cg_forceCrosshair = qtrue;
					}
				}
				// No?  Check for force-push/pullable doors and func_statics
				else if ( traceEnt->s.eType == ET_MOVER )
				{//hit a mover
					if ( !Q_stricmp( "func_door", traceEnt->classname ) )
					{//it's a func_door
						if ( traceEnt->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/ )
						{//it's force-usable
							if ( cg_entities[0].gent->client->ps.forcePowerLevel[FP_PULL] || cg_entities[0].gent->client->ps.forcePowerLevel[FP_PUSH] )
							{//player has push or pull
								float maxRange;
								if ( cg_entities[0].gent->client->ps.forcePowerLevel[FP_PULL] > cg_entities[0].gent->client->ps.forcePowerLevel[FP_PUSH] )
								{//use the better range
									maxRange = forcePushPullRadius[cg_entities[0].gent->client->ps.forcePowerLevel[FP_PULL]];
								}
								else
								{//use the better range
									maxRange = forcePushPullRadius[cg_entities[0].gent->client->ps.forcePowerLevel[FP_PUSH]];
								}
								if ( maxRange >= trace.fraction * 2048 )
								{//actually close enough to use one of our force powers on it
									cg_forceCrosshair = qtrue;
								}
							}
						}
					}
					else if ( !Q_stricmp( "func_static", traceEnt->classname ) )
					{//it's a func_static
						if ( (traceEnt->spawnflags & 1/*F_PUSH*/) && (traceEnt->spawnflags & 2/*F_PULL*/) )
						{//push or pullable
							float maxRange;
							if ( cg_entities[0].gent->client->ps.forcePowerLevel[FP_PULL] > cg_entities[0].gent->client->ps.forcePowerLevel[FP_PUSH] )
							{//use the better range
								maxRange = forcePushPullRadius[cg_entities[0].gent->client->ps.forcePowerLevel[FP_PULL]];
							}
							else
							{//use the better range
								maxRange = forcePushPullRadius[cg_entities[0].gent->client->ps.forcePowerLevel[FP_PUSH]];
							}
							if ( maxRange >= trace.fraction * 2048 )
							{//actually close enough to use one of our force powers on it
								cg_forceCrosshair = qtrue;
							}
						}
						else if ( (traceEnt->spawnflags & 1/*F_PUSH*/) )
						{//pushable only
							if ( forcePushPullRadius[cg_entities[0].gent->client->ps.forcePowerLevel[FP_PUSH]] >= trace.fraction * 2048 )
							{//actually close enough to use force push on it
								cg_forceCrosshair = qtrue;
							}
						}
						else if ( (traceEnt->spawnflags & 2/*F_PULL*/) )
						{//pullable only
							if ( forcePushPullRadius[cg_entities[0].gent->client->ps.forcePowerLevel[FP_PULL]] >= trace.fraction * 2048 )
							{//actually close enough to use force pull on it
								cg_forceCrosshair = qtrue;
							}
						}
					}
				}
			}
		}
	}
	if ( !cg_forceCrosshair )
	{
		if ( cg_dynamicCrosshair.integer )
		{//100% accurate
			vec3_t d_f, d_rt, d_up;
			// If you're riding a vehicle and not being drawn.
			if ( ( pVeh = G_IsRidingVehicle( cg_entities[0].gent ) ) != NULL && cg_entities[0].currentState.eFlags & EF_NODRAW )
			{
				VectorCopy( cg_entities[pVeh->m_pParentEntity->s.number].lerpOrigin, start );
				AngleVectors( cg_entities[pVeh->m_pParentEntity->s.number].lerpAngles, d_f, d_rt, d_up );
			}
			else if ( cg.snap->ps.weapon == WP_NONE || cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_STUN_BATON )
			{
				if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
				{//in camera ent view
					ignoreEnt = cg.snap->ps.viewEntity;
					if ( g_entities[cg.snap->ps.viewEntity].client )
					{
						VectorCopy( g_entities[cg.snap->ps.viewEntity].client->renderInfo.eyePoint, start );
					}
					else
					{
						VectorCopy( cg_entities[cg.snap->ps.viewEntity].lerpOrigin, start );
					}
					AngleVectors( cg_entities[cg.snap->ps.viewEntity].lerpAngles, d_f, d_rt, d_up );
				}
				else
				{
					VectorCopy( g_entities[0].client->renderInfo.eyePoint, start );
					AngleVectors( cg_entities[0].lerpAngles, d_f, d_rt, d_up );
				}
			}
			else
			{
				extern void CalcMuzzlePoint( gentity_t *const ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint, float lead_in );
				AngleVectors( cg_entities[0].lerpAngles, d_f, d_rt, d_up );
				CalcMuzzlePoint( &g_entities[0], d_f, d_rt, d_up, start , 0 );
			}
			//VectorCopy( g_entities[0].client->renderInfo.muzzlePoint, start );
			//FIXME: increase this?  Increase when zoom in?
			VectorMA( start, 4096, d_f, end );//was 8192
		}
		else
		{//old way
			VectorCopy( cg.refdef.vieworg, start );
			//FIXME: increase this?  Increase when zoom in?
			VectorMA( start, 131072, cg.refdef.viewaxis[0], end );//was 8192
		}
		//YES!  This is very very bad... but it works!  James made me do it.  Really, he did.  Blame James.
		gi.trace( &trace, start, vec3_origin, vec3_origin, end, 
			ignoreEnt, MASK_OPAQUE|CONTENTS_TERRAIN|CONTENTS_SHOTCLIP|CONTENTS_BODY|CONTENTS_ITEM, G2_NOCOLLIDE, 10 );// ); took out CONTENTS_SOLID| so you can target people through glass.... took out CONTENTS_CORPSE so disintegrated guys aren't shown, could just remove their body earlier too...

		/*
		CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
			cg.snap->ps.clientNum, MASK_PLAYERSOLID|CONTENTS_CORPSE|CONTENTS_ITEM );
		*/
		//FIXME: pick up corpses
		if ( trace.startsolid || trace.allsolid )
		{
			// trace should not be allowed to pick up anything if it started solid.  I tried actually moving the trace start back, which also worked, 
			//	but the dynamic cursor drawing caused it to render around the clip of the gun when I pushed the blaster all the way into a wall.
			//	It looked quite horrible...but, if this is bad for some reason that I don't know
			trace.entityNum = ENTITYNUM_NONE;
		}

		traceEnt = &g_entities[trace.entityNum];
	}
	

	// if the object is "dead", don't show it
/*	if ( cg.crosshairClientNum && g_entities[cg.crosshairClientNum].health <= 0 )
	{
		cg.crosshairClientNum = 0;
		return;
	}
*/
	//draw crosshair at endpoint
	CG_DrawCrosshair( trace.endpos );

	g_crosshairEntNum = trace.entityNum;
	g_crosshairEntDist = 4096*trace.fraction;

	if ( !traceEnt )
	{
		//not looking at anything
		g_crosshairSameEntTime = 0;
		g_crosshairEntTime = 0;
	}
	else
	{//looking at a valid ent
		//store the distance
		if ( trace.entityNum != g_crosshairEntNum )
		{//new crosshair ent
			g_crosshairSameEntTime = 0;
		}
		else if ( g_crosshairEntDist < 256 )
		{//close enough to start counting how long you've been looking
			g_crosshairSameEntTime += cg.frametime;
		}
		//remember the last time you looked at the person
		g_crosshairEntTime = cg.time;
	}

	if ( !traceEnt )
	{
		if ( traceEnt && scanAll )
		{
		}
		else
		{
			return;
		}
	}

	// if the player is in fog, don't show it
	content = cgi_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) 
	{
		return;
	}

	// if the player is cloaked, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_CLOAKED )) 
	{
		return;
	}

	// update the fade timer
	if ( cg.crosshairClientNum != trace.entityNum )
	{
		infoStringCount = 0;
	}

	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) 
{
	qboolean	scanAll = qfalse;
	centity_t	*player = &cg_entities[0];

	if ( cg_dynamicCrosshair.integer )
	{
		// still need to scan for dynamic crosshair
		CG_ScanForCrosshairEntity( scanAll );
		return;
	}

	if ( !player->gent )
	{
		return;
	}

	if ( !player->gent->client )
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	// This is currently being called by the rocket tracking code, so we don't necessarily want to do duplicate traces :)
	CG_ScanForCrosshairEntity( scanAll );
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int startx = icon_size*2+16;
	int starty = SCREEN_HEIGHT - icon_size*2;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.zoomMode)
	{ //don't display over zoom mask
		return;
	}

	/*
	//draw icon for duration powers so we know what powers are active cuttently
	int i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if ((cg.snap->ps.forcePowersActive & (1 << forcePowerSorted[i])) &&
			CG_IsDurationPower(forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			startx += (icon_size+2); //+2 for spacing
			if ((startx+icon_size) >= SCREEN_WIDTH-80)
			{
				startx = icon_size*2+16;
				starty += (icon_size+2);
			}
		}

		i++;
	}
	*/

	//additionally, draw an icon force force rage recovery
	if (cg.snap->ps.forceRageRecoveryTime > cg.time)
	{
		CG_DrawPic( startx, starty, endx, endy, cgs.media.rageRecShader);
	}
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking( int lockEntNum, int lockTime )
//--------------------------------------------------------------
{
	gentity_t *gent = &g_entities[lockEntNum];

	if ( !gent )
	{
		return;
	}

	int		cx, cy;
	vec3_t	org;
	static	int oldDif = 0;

	VectorCopy( gent->currentOrigin, org );
	org[2] += (gent->mins[2] + gent->maxs[2]) * 0.5f;

	if ( CG_WorldCoordToScreenCoord( org, &cx, &cy ))
	{
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance( gent->currentOrigin, cg.refdef.vieworg ) / 1024.0f; 
		
		if ( cg.zoomMode > 0 )
		{
			if ( cg.overrides.active & CG_OVERRIDE_FOV )
			{
				sz -= ( cg.overrides.fov - cg_zoomFov ) / 80.0f;
			}
			else
			{
				sz -= ( cg_fov.value - cg_zoomFov ) / 80.0f;
			}
		}

		if ( sz > 1.0f )
		{
			sz = 1.0f;
		}
		else if ( sz < 0.0f )
		{
			sz = 0.0f;
		}

		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;

		vec4_t color={0.0f,0.0f,0.0f,0.0f};

		cy += sz * 0.5f;
		
		// well now, take our current lock time and divide that by 8 wedge slices to get the current lock amount
		int dif = ( cg.time - g_rocketLockTime ) / ( 1200.0f / 8.0f );

		if ( dif < 0 )
		{
			oldDif = 0;
			return;
		}
		else if ( dif > 8 )
		{
			dif = 8;
		}

		// do sounds
		if ( oldDif != dif )
		{
			if ( dif == 8 )
			{
				cgi_S_StartSound( org, 0, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/rocket/lock.wav" ));
			}
			else
			{
				cgi_S_StartSound( org, 0, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/rocket/tick.wav" ));
			}
		}

		oldDif = dif;

		for ( int i = 0; i < dif; i++ )
		{
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.1f * i + 0.2f;

			cgi_R_SetColor( color );

			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic( cx - sz, cy - sz, sz, sz, i * 45.0f, cgi_R_RegisterShaderNoMip( "gfx/2d/wedge" ));
		}

		// we are locked and loaded baby
		if ( dif == 8 )
		{
			color[0] = color[1] = color[2] = sin( cg.time * 0.05f ) * 0.5f + 0.5f;
			color[3] = 1.0f; // this art is additive, so the alpha value does nothing

			cgi_R_SetColor( color );

			CG_DrawPic( cx - sz, cy - sz * 2, sz * 2, sz * 2, cgi_R_RegisterShaderNoMip( "gfx/2d/lock" ));
		}
	}
}


//------------------------------------
static void CG_RunRocketLocking( void )
//------------------------------------
{
	centity_t	*player = &cg_entities[0];

	// Only bother with this when the player is holding down the alt-fire button of the rocket launcher
	if ( player->currentState.weapon == WP_ROCKET_LAUNCHER )
	{
		if ( player->currentState.eFlags & EF_ALT_FIRING )
		{
			CG_ScanForRocketLock();

			if ( g_rocketLockEntNum > 0 && g_rocketLockEntNum < ENTITYNUM_WORLD && g_rocketLockTime > 0 )
			{
				CG_DrawRocketLocking( g_rocketLockEntNum, g_rocketLockTime );
			}
		}
		else
		{
			// disengage any residual locking
			g_rocketLockEntNum = ENTITYNUM_WORLD;
			g_rocketLockTime = 0;
		}
	}
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	CG_DrawScoreboard();
}


/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );

	w = cgi_R_Font_StrLenPixels(s, cgs.media.qhFontMedium, 1.0f);	
	cgi_R_Font_DrawString(635 - w, y+2, s, colorTable[CT_LTGOLD1], cgs.media.qhFontMedium, -1, 1.0f);

	return y + BIGCHAR_HEIGHT + 10;
}


/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16
static float CG_DrawFPS( float y ) {
	char		*s;
	static unsigned short previousTimes[FPS_FRAMES];
	static unsigned short index;
	static int	previous, lastupdate;
	int		t, i, fps, total;
	unsigned short frameTime;
	const int		xOffset = 0;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = cgi_Milliseconds();
	frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 50)	//don't sample faster than this
	{
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	// average multiple frames together to smooth changes out a bit
	total = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
		total += previousTimes[i];
	}
	if ( !total ) {
		total = 1;
	}
	fps = 1000 * FPS_FRAMES / total;

	s = va( "%ifps", fps );
	const int w = cgi_R_Font_StrLenPixels(s, cgs.media.qhFontMedium, 1.0f);	
	cgi_R_Font_DrawString(635-xOffset - w, y+2, s, colorTable[CT_LTGOLD1], cgs.media.qhFontMedium, -1, 1.0f);

	return y + BIGCHAR_HEIGHT + 10;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;

	seconds = cg.time / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );

	w = cgi_R_Font_StrLenPixels(s, cgs.media.qhFontMedium, 1.0f);	
	cgi_R_Font_DrawString(635 - w, y+2, s, colorTable[CT_LTGOLD1], cgs.media.qhFontMedium, -1, 1.0f);

	return y + BIGCHAR_HEIGHT + 10;
}


/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	char text[1024]={0};
	int			w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( weaponData[cg.snap->ps.weapon].ammoIndex == AMMO_NONE )
	{//doesn't use ammo, so no warning
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		cgi_SP_GetStringTextString( "SP_INGAME_INSUFFICIENTENERGY", text, sizeof(text) );
	} else {
		return;
		//s = "LOW AMMO WARNING";
	}

	w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);	
	cgi_R_Font_DrawString(320 - w/2, 64, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 1.0f);
}

//---------------------------------------
static qboolean CG_RenderingFromMiscCamera()
{
	//centity_t *cent;

	//cent = &cg_entities[cg.snap->ps.clientNum];

	if ( cg.snap->ps.viewEntity > 0 &&
		cg.snap->ps.viewEntity < ENTITYNUM_WORLD )// cent && cent->gent && cent->gent->client && cent->gent->client->ps.viewEntity)
	{
		// Only check viewEntities
		if ( !Q_stricmp( "misc_camera", g_entities[cg.snap->ps.viewEntity].classname ))
		{
			// Only doing a misc_camera, so check health.
			if ( g_entities[cg.snap->ps.viewEntity].health > 0 )
			{
				CG_DrawPic( 0, 0, 640, 480, cgi_R_RegisterShader( "gfx/2d/workingCamera" ));
			}
			else
			{
				CG_DrawPic( 0, 0, 640, 480, cgi_R_RegisterShader( "gfx/2d/brokenCamera" ));
			}
			// don't render other 2d stuff
			return qtrue;
		}
		else if ( !Q_stricmp( "misc_panel_turret", g_entities[cg.snap->ps.viewEntity].classname ))
		{
			// could do a panel turret screen overlay...this is a cheesy placeholder
			CG_DrawPic( 30, 90, 128, 300, cgs.media.turretComputerOverlayShader );
			CG_DrawPic( 610, 90, -128, 300, cgs.media.turretComputerOverlayShader );
		}
		else
		{
			// FIXME: make sure that this assumption is correct...because I'm assuming that I must be a droid.
			CG_DrawPic( 0, 0, 640, 480, cgi_R_RegisterShader( "gfx/2d/droid_view" ));
		}
	}

	// not in misc_camera, render other stuff.
	return qfalse;
}

qboolean cg_usingInFrontOf = qfalse;
qboolean CanUseInfrontOf(gentity_t*);
static void CG_UseIcon()
{
	cg_usingInFrontOf = CanUseInfrontOf(cg_entities[cg.snap->ps.clientNum].gent);
	if (cg_usingInFrontOf)
	{
		cgi_R_SetColor( NULL );
		CG_DrawPic( 50, 285, 64, 64, cgs.media.useableHint );
	}
}

static void CG_Draw2DScreenTints( void )
{
	float	rageTime, rageRecTime, absorbTime, protectTime;
	vec4_t	hcolor;
	//force effects
	if (cg.snap->ps.forcePowersActive & (1 << FP_RAGE))
	{
		if (!cgRageTime)
		{
			cgRageTime = cg.time;
		}
		
		rageTime = (float)(cg.time - cgRageTime);
		
		rageTime /= 9000;
		
		if ( rageTime < 0 )
		{
			rageTime = 0;
		}
		if ( rageTime > 0.15f )
		{
			rageTime = 0.15f;
		}
		
		hcolor[3] = rageTime;
		hcolor[0] = 0.7f;
		hcolor[1] = 0;
		hcolor[2] = 0;
		
		if (!cg.renderingThirdPerson)
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		
		cgRageFadeTime = 0;
		cgRageFadeVal = 0;
	}
	else if (cgRageTime)
	{
		if (!cgRageFadeTime)
		{
			cgRageFadeTime = cg.time;
			cgRageFadeVal = 0.15f;
		}
		
		rageTime = cgRageFadeVal;
		
		cgRageFadeVal -= (cg.time - cgRageFadeTime)*0.000005;
		
		if (rageTime < 0)
		{
			rageTime = 0;
		}
		if ( rageTime > 0.15f )
		{
			rageTime = 0.15f;
		}
		
		if ( cg.snap->ps.forceRageRecoveryTime > cg.time )
		{
			float checkRageRecTime = rageTime;
			
			if ( checkRageRecTime < 0.15f )
			{
				checkRageRecTime = 0.15f;
			}
			
			hcolor[3] = checkRageRecTime;
			hcolor[0] = rageTime*4;
			if ( hcolor[0] < 0.2f )
			{
				hcolor[0] = 0.2f;
			}
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;
		}
		else
		{
			hcolor[3] = rageTime;
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		}
		
		if (!cg.renderingThirdPerson && rageTime)
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		else
		{
			if (cg.snap->ps.forceRageRecoveryTime > cg.time)
			{
				hcolor[3] = 0.15f;
				hcolor[0] = 0.2f;
				hcolor[1] = 0.2f;
				hcolor[2] = 0.2f;
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
			}
			cgRageTime = 0;
		}
	}
	else if (cg.snap->ps.forceRageRecoveryTime > cg.time)
	{
		if (!cgRageRecTime)
		{
			cgRageRecTime = cg.time;
		}
		
		rageRecTime = (float)(cg.time - cgRageRecTime);
		
		rageRecTime /= 9000;
		
		if ( rageRecTime < 0.15f )//0)
		{
			rageRecTime = 0.15f;//0;
		}
		if ( rageRecTime > 0.15f )
		{
			rageRecTime = 0.15f;
		}
		
		hcolor[3] = rageRecTime;
		hcolor[0] = 0.2f;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.2f;
		
		if ( !cg.renderingThirdPerson )
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		
		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;
	}
	else if (cgRageRecTime)
	{
		if (!cgRageRecFadeTime)
		{
			cgRageRecFadeTime = cg.time;
			cgRageRecFadeVal = 0.15f;
		}
		
		rageRecTime = cgRageRecFadeVal;
		
		cgRageRecFadeVal -= (cg.time - cgRageRecFadeTime)*0.000005;
		
		if (rageRecTime < 0)
		{
			rageRecTime = 0;
		}
		if ( rageRecTime > 0.15f )
		{
			rageRecTime = 0.15f;
		}
		
		hcolor[3] = rageRecTime;
		hcolor[0] = 0.2f;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.2f;
		
		if (!cg.renderingThirdPerson && rageRecTime)
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		else
		{
			cgRageRecTime = 0;
		}
	}
	
	if (cg.snap->ps.forcePowersActive & (1 << FP_ABSORB))
	{
		if (!cgAbsorbTime)
		{
			cgAbsorbTime = cg.time;
		}
		
		absorbTime = (float)(cg.time - cgAbsorbTime);
		
		absorbTime /= 9000;
		
		if ( absorbTime < 0 )
		{
			absorbTime = 0;
		}
		if ( absorbTime > 0.15f )
		{
			absorbTime = 0.15f;
		}
		
		hcolor[3] = absorbTime/2;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0.7f;
		
		if ( !cg.renderingThirdPerson )
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		
		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;
	}
	else if (cgAbsorbTime)
	{
		if (!cgAbsorbFadeTime)
		{
			cgAbsorbFadeTime = cg.time;
			cgAbsorbFadeVal = 0.15f;
		}
		
		absorbTime = cgAbsorbFadeVal;
		
		cgAbsorbFadeVal -= (cg.time - cgAbsorbFadeTime)*0.000005;
		
		if ( absorbTime < 0 )
		{
			absorbTime = 0;
		}
		if ( absorbTime > 0.15f )
		{
			absorbTime = 0.15f;
		}
		
		hcolor[3] = absorbTime/2;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0.7f;
		
		if ( !cg.renderingThirdPerson && absorbTime )
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		else
		{
			cgAbsorbTime = 0;
		}
	}
	
	if (cg.snap->ps.forcePowersActive & (1 << FP_PROTECT))
	{
		if (!cgProtectTime)
		{
			cgProtectTime = cg.time;
		}
		
		protectTime = (float)(cg.time - cgProtectTime);
		
		protectTime /= 9000;
		
		if (protectTime < 0)
		{
			protectTime = 0;
		}
		if ( protectTime > 0.15f )
		{
			protectTime = 0.15f;
		}
		
		hcolor[3] = protectTime/2;
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;
		
		if ( !cg.renderingThirdPerson )
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		
		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;
	}
	else if ( cgProtectTime )
	{
		if ( !cgProtectFadeTime )
		{
			cgProtectFadeTime = cg.time;
			cgProtectFadeVal = 0.15f;
		}
		
		protectTime = cgProtectFadeVal;
		
		cgProtectFadeVal -= (cg.time - cgProtectFadeTime)*0.000005;
		
		if (protectTime < 0)
		{
			protectTime = 0;
		}
		if (protectTime > 0.15f)
		{
			protectTime = 0.15f;
		}
		
		hcolor[3] = protectTime/2;
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;
		
		if ( !cg.renderingThirdPerson && protectTime )
		{
			CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
		}
		else
		{
			cgProtectTime = 0;
		}
	}

	if ( (cg.refdef.viewContents&CONTENTS_LAVA) )
	{//tint screen red
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.5 + (0.15f*sin( phase ));
		hcolor[0] = 0.7f;
		hcolor[1] = 0;
		hcolor[2] = 0;
		
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
	}
	else if ( (cg.refdef.viewContents&CONTENTS_SLIME) )
	{//tint screen green
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.4 + (0.1f*sin( phase ));
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;
		
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
	}
	else if ( (cg.refdef.viewContents&CONTENTS_WATER) )
	{//tint screen light blue -- FIXME: check to see if in fog? 
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.3 + (0.05f*sin( phase ));
		hcolor[0] = 0;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.8f;
		
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor  );
	}
}
/*
=================
CG_Draw2D
=================
*/
extern void CG_SaberClashFlare( void );
static void CG_Draw2D( void ) 
{
	char	text[1024]={0};
	int		w,y_pos;
	centity_t *cent = &cg_entities[cg.snap->ps.clientNum];

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) 
	{
		return;
	}

	if ( cg_draw2D.integer == 0 ) 
	{
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) 
	{
		CG_DrawIntermission();
		return;
	}

	CG_Draw2DScreenTints();

	//end credits
	if (cg_endcredits.integer)
	{
		if (!CG_Credits_Draw())
		{
			CG_DrawCredits();	// will probably get rid of this soon
		}
	}

	CGCam_DrawWideScreen();

	CG_DrawBatteryCharge();

	if (cg.snap->ps.forcePowersActive || cg.snap->ps.forceRageRecoveryTime > cg.time)
	{
		CG_DrawActivePowers();
	}

	// Draw this before the text so that any text won't get clipped off
	if ( !in_camera )
	{
		CG_DrawZoomMask();
	}

	CG_DrawScrollText();
	CG_DrawCaptionText(); 

	if ( in_camera )
	{//still draw the saber clash flare, but nothing else
		CG_SaberClashFlare();
		return;
	}

	if ( CG_RenderingFromMiscCamera())
	{
		// purposely doing an early out when in a misc_camera, change it if needed.
		
		// allowing center print when in camera mode, probably just an alpha thing - dmv
		CG_DrawCenterString();
		return;
	}

	if ( (cg.snap->ps.forcePowersActive&(1<<FP_SEE)) )
	{//force sight is on
		//indicate this with sight cone thingy
		CG_DrawPic( 0, 0, 640, 480, cgi_R_RegisterShader( "gfx/2d/jsense" ));
		CG_DrawHealthBars();
	}
	else if ( cg_debugHealthBars.integer )
	{
		CG_DrawHealthBars();
	}


	// don't draw any status if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] > 0 ) 
	{
		if ( !(cent->gent && cent->gent->s.eFlags & (EF_LOCKED_TO_WEAPON )))//|EF_IN_ATST
		{
			//CG_DrawIconBackground();
		}

		CG_DrawWeaponSelect();

		if ( cg.zoomMode == 0 )
		{
			CG_DrawStats();
		}
		CG_DrawAmmoWarning();

		//CROSSHAIR is now done from the crosshair ent trace
		//if ( !cg.renderingThirdPerson && !cg_dynamicCrosshair.integer ) // disruptor draws it's own crosshair artwork; binocs draw nothing; third person draws its own crosshair
		//{
		//	CG_DrawCrosshair( NULL );
		//}


		CG_DrawCrosshairNames();

		CG_RunRocketLocking();

		CG_DrawInventorySelect();

		CG_DrawForceSelect();

		CG_DrawPickupItem();

		CG_UseIcon();
	}
	CG_SaberClashFlare();

	float y = 0;
	if (cg_drawSnapshot.integer) {
		y=CG_DrawSnapshot(y);
	} 
	if (cg_drawFPS.integer) {
		y=CG_DrawFPS(y);
	} 
	if (cg_drawTimer.integer) {
		y=CG_DrawTimer(y);
	}

	// don't draw center string if scoreboard is up
	if ( !CG_DrawScoreboard() ) {
		CG_DrawCenterString();
	}

/*	if (cg.showInformation)
	{
//		CG_DrawMissionInformation();
	}
	else 
*/	
	if (missionInfo_Updated)
	{	
		if (cg.predicted_player_state.pm_type != PM_DEAD)
		{
			// Was a objective given?
/*			if ((cg_updatedDataPadForcePower.integer) || (cg_updatedDataPadObjective.integer))
			{
				// How long has the game been running? If within 15 seconds of starting, throw up the datapad.
				if (cg.dataPadLevelStartTime>cg.time)
				{
					// Make it pop up
					if (!in_camera)
					{
						cgi_SendConsoleCommand( "datapad" );
						cg.dataPadLevelStartTime=cg.time;	//and don't do it again this level!
					}
				}
			}
*/
			if (!cg.missionInfoFlashTime)
			{
				cg.missionInfoFlashTime	= cg.time + cg_missionInfoFlashTime.integer;
			}

			if (cg.missionInfoFlashTime < cg.time)	// Time's up.  They didn't read it.
			{
				cg.missionInfoFlashTime = 0;
				missionInfo_Updated = qfalse;

				CG_ClearDataPadCvars();
			}

			cgi_SP_GetStringTextString( "SP_INGAME_NEW_OBJECTIVE_INFO", text, sizeof(text) );
			
			int x_pos = 0;
			y_pos = 20;
			w = cgi_R_Font_StrLenPixels(text,cgs.media.qhFontMedium, 1.0f);
			x_pos = (SCREEN_WIDTH/2)-(w/2);
			cgi_R_Font_DrawString(x_pos, y_pos, text,  colorTable[CT_LTRED1], cgs.media.qhFontMedium, -1, 1.0f);
		}
	}

	if (cg.weaponPickupTextTime	> cg.time )
	{
		int x_pos = 0;
		y_pos = 5;
		gi.Cvar_VariableStringBuffer( "cg_WeaponPickupText", text, sizeof(text) );

		w = cgi_R_Font_StrLenPixels(text,cgs.media.qhFontMedium, 0.8f);
		x_pos = (SCREEN_WIDTH/2)-(w/2);

		cgi_R_Font_DrawString(x_pos, y_pos, text,  colorTable[CT_WHITE], cgs.media.qhFontMedium, -1, 0.8f);
	}
}

/*
===================
CG_DrawIconBackground

Choose the proper background for the icons, scale it depending on if your opening or
closing the icon section of the HU
===================
*/
void CG_DrawIconBackground(void)
{
	int				backgroundXPos,backgroundYPos;
	int				backgroundWidth,backgroundHeight;
	qhandle_t		background;
	const float		shutdownTime = 130.0f;

	if ( cg_hudFiles.integer )
	{ //simple hud
		return;
	}

	// Are we in zoom mode or the HUD is turned off?
	if (( cg.zoomMode != 0 ) || !( cg_drawHUD.integer ))
	{
		return;
	}

	if ((cg.snap->ps.viewEntity>0 && cg.snap->ps.viewEntity<ENTITYNUM_WORLD))
	{
		return;
	}

	// Get size and location of bakcround specified in the HUD.MENU file
	if (!cgi_UI_GetMenuInfo("iconbackground",&backgroundXPos,&backgroundYPos,&backgroundWidth,&backgroundHeight))
	{
		return;
	}

	// Use inventory background?
	if (((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time) || (cgs.media.currentBackground == ICON_INVENTORY))	
	{
		background = cgs.media.inventoryIconBackground;
	}
	// Use weapon background?
	else if (((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time) || (cgs.media.currentBackground == ICON_WEAPONS))	
	{
		background = 0;
		//background = cgs.media.weaponIconBackground;
	}
	// Use force background?
	else 	
	{
		background = cgs.media.forceIconBackground;
	}

	// Time is up, shutdown the icon section of the HUD
	if ((cg.iconSelectTime+WEAPON_SELECT_TIME)<cg.time)
	{
		// Scale background down as it goes away
		if (background && cg.iconHUDActive)		
		{
			cg.iconHUDPercent = (cg.time - (cg.iconSelectTime+WEAPON_SELECT_TIME))/ shutdownTime;
			cg.iconHUDPercent = 1.0f - cg.iconHUDPercent;

			if (cg.iconHUDPercent<0.0f)
			{
				cg.iconHUDActive = qfalse;
				cg.iconHUDPercent=0.f;
			}

			float holdFloat = (float) backgroundHeight;
			backgroundHeight = (int) (holdFloat*cg.iconHUDPercent);
			CG_DrawPic( backgroundXPos, backgroundYPos, backgroundWidth, -backgroundHeight, background);	// Top half
			CG_DrawPic( backgroundXPos, backgroundYPos,backgroundWidth, backgroundHeight, background);	// Bottom half
		}
		return;
	}

	// Scale background up as it comes up
	if (!cg.iconHUDActive)
	{
		cg.iconHUDPercent = (cg.time - cg.iconSelectTime)/ shutdownTime;

		// Calc how far into opening sequence we are
		if (cg.iconHUDPercent>1.0f)
		{
			cg.iconHUDActive = qtrue;
			cg.iconHUDPercent=1.0f;
		}
		else if (cg.iconHUDPercent<0.0f)
		{
			cg.iconHUDPercent=0.0f;
		}
	}
	else
	{
		cg.iconHUDPercent=1.0f;
	}

	// Print the background
	if (background)
	{
		cgi_R_SetColor( colorTable[CT_WHITE] );					
		float holdFloat = (float) backgroundHeight;
		backgroundHeight = (int) (holdFloat*cg.iconHUDPercent);
		CG_DrawPic( backgroundXPos, backgroundYPos, backgroundWidth, -backgroundHeight, background);	// Top half
		CG_DrawPic( backgroundXPos, backgroundYPos,backgroundWidth, backgroundHeight, background);	// Bottom half
	}
	if ((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time)	
	{
		cgs.media.currentBackground = ICON_INVENTORY;
	}
	else if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time)	 
	{
		cgs.media.currentBackground = ICON_WEAPONS;
	}
	else 
	{
		cgs.media.currentBackground = ICON_FORCE;
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	//FIXME: these globals done once at start of frame for various funcs
	AngleVectors (cg.refdefViewAngles, vfwd, vright, vup);
	VectorCopy( vfwd, vfwd_n );
	VectorCopy( vright, vright_n );
	VectorCopy( vup, vup_n );
	VectorNormalize( vfwd_n );
	VectorNormalize( vright_n );
	VectorNormalize( vup_n );

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	if ( cg.zoomMode == 3 && cg.snap->ps.batteryCharge ) // doing the Light amp goggles thing
	{
		cgi_R_LAGoggles();
	}

	if ( (cg.snap->ps.forcePowersActive&(1<<FP_SEE)) )
	{
		cg.refdef.rdflags |= RDF_ForceSightOn;
	}

	cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	// draw 3D view
	cgi_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// draw status bar and other floating elements
	CG_Draw2D();

}

