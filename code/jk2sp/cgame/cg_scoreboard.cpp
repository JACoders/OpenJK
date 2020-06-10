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
#include "../game/objectives.h"
#include "../game/b_local.h"

#define	SCOREBOARD_WIDTH	(26*BIGCHAR_WIDTH)


/*
static void Scoreboard_Draw( void )
{
	vec4_t	newColor;
/*
		player = g_entities[0];
		if( player->client->ps.persistant[PERS_ACCURACY_SHOTS] ) {
			accuracy = player->client->ps.persistant[PERS_ACCURACY_HITS] * 100 / player->client->ps.persistant[PERS_ACCURACY_SHOTS];
		}
*/
/*	cg.LCARSTextTime = 0;	//	Turn off LCARS screen

	// Background
	newColor[0] = colorTable[CT_BLACK][0];
	newColor[1] = colorTable[CT_BLACK][1];
	newColor[2] = colorTable[CT_BLACK][2];
	newColor[3] = 0.5;
	cgi_R_SetColor(newColor);
	CG_DrawPic( 137, 73, 475, 300, cgs.media.whiteShader);	// Background
	CG_DrawPic( 120, 99,  18, 256, cgs.media.whiteShader);	// Background
	CG_DrawPic(  40, 94,  66, 266, cgs.media.whiteShader);	// Background

	// Right side box
	cgi_R_SetColor( colorTable[CT_DKBROWN1]);
	CG_DrawPic( 120, 354,  32, 32, cgs.media.status_corner_16_18);
	CG_DrawPic(  94, 356,  16, 32, cgs.media.status_corner_8_16_b);
	CG_DrawPic(  94, 73,  16, 32, cgs.media.status_corner_8_22);

	CG_DrawPic(135,73,  302, 22, cgs.media.whiteShader);	// Top

	CG_DrawPic(120, 100,  18, 12, cgs.media.whiteShader);	// Middle Top
	CG_DrawPic(120, 353,  18, 4,  cgs.media.whiteShader);	// Middle Bottom
  
	CG_DrawPic(130,357,  482, 18, cgs.media.whiteShader);	// Bottom

	// Left side box
	cgi_R_SetColor( colorTable[CT_DKBROWN1]);
	CG_DrawPic(40,73,  56, 22, cgs.media.whiteShader);	// Top

	CG_DrawPic(98,95,  8, 17, cgs.media.whiteShader);		// Middle Top
	CG_DrawPic(98,353, 8, 7, cgs.media.whiteShader);		// Middle Bottom

	CG_DrawPic(40,357, 58, 18, cgs.media.whiteShader);	// Bottom

	CG_DrawProportionalString( 356, 208, "%", UI_RIGHT | UI_SMALLFONT, colorTable[CT_LTPURPLE1] );
	CG_DrawProportionalString( 610, 72, ingame_text[IGT_MISSIONANALYSIS],UI_RIGHT| CG_BIGFONT, colorTable[CT_LTORANGE] );

	CG_PrintScreenGraphics(statsmenu_graphics,SMG_MAX);

}
*/



/*
=================
CG_MissionFailed
=================
*/
int statusTextIndex = -1;
void CG_MissionFailed(void)
{
	char text[1024]={0};
	int w;
	int y = 230;

		cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED", text, sizeof(text) );

	w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontMedium, 1.2f);	
		cgi_R_Font_DrawString(320 - w/2, y, text, colorTable[CT_HUD_RED], cgs.media.qhFontMedium, -1, 1.2f);

	switch (statusTextIndex)
	{
		case -1:	//Our HERO DIED!!!
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_KYLE", text, sizeof(text) );
			break;
		case MISSIONFAILED_JAN:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_JAN", text, sizeof(text) );
			break;
		case MISSIONFAILED_LUKE:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_LUKE", text, sizeof(text) );
			break;
		case MISSIONFAILED_LANDO:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_LANDO", text, sizeof(text) );
			break;
		case MISSIONFAILED_R5D2:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_R5D2", text, sizeof(text) );
			break;
		case MISSIONFAILED_WARDEN:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_WARDEN", text, sizeof(text) );
			break;
		case MISSIONFAILED_PRISONERS:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_PRISONERS", text, sizeof(text) );
			break;
		case MISSIONFAILED_EMPLACEDGUNS:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_EMPLACEDGUNS", text, sizeof(text) );
			break;
		case MISSIONFAILED_LADYLUCK:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_LADYLUCK", text, sizeof(text) );
			break;
		case MISSIONFAILED_KYLECAPTURE:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_KYLECAPTURE", text, sizeof(text) );
			break;
		case MISSIONFAILED_TOOMANYALLIESDIED:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_TOOMANYALLIESDIED", text, sizeof(text) );
			break;
		default:
			cgi_SP_GetStringTextString( "INGAME_MISSIONFAILED_UNKNOWN", text, sizeof(text) );
			break;
	}

	w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontMedium, 1.2f);	
		cgi_R_Font_DrawString(320 - w/2, y+30, text, colorTable[CT_HUD_RED], cgs.media.qhFontMedium, -1, 1.2f);

		cgi_SP_GetStringTextString( "INGAME_RELOADMISSION", text, sizeof(text) );
	w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);	
		cgi_R_Font_DrawString(320 - w/2, 450, text, colorTable[CT_CYAN], cgs.media.qhFontSmall, -1, 1.0f);

}


/*
=================
CG_MissionCompletion
=================
*/
void CG_MissionCompletion(void)
{
	char text[1024]={0};
	int w,x,y;
	const int pad = 18;

	cgi_SP_GetStringTextString( "INGAME_MISSIONCOMPLETION", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontMedium, 1.2f);	
	cgi_R_Font_DrawString(320 - w/2, 53, text, colorTable[CT_LTGOLD1], cgs.media.qhFontMedium, -1, 1.2f);

	x = 75;
	y =86;
	cgi_SP_GetStringTextString( "INGAME_SECRETAREAS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,    y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_SP_GetStringTextString( "INGAME_SECRETAREAS_OF", text, sizeof(text) );
	cgi_R_Font_DrawString(x+w,  y, va("%d %s %d", 
										cg_entities[0].gent->client->sess.missionStats.secretsFound, 
										text,
										cg_entities[0].gent->client->sess.missionStats.totalSecrets
										),
							colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_ENEMIESKILLED", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w,y, va("%d",cg_entities[0].gent->client->sess.missionStats.enemiesKilled), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);
	/*
	cgi_SP_GetStringTextString( "INGAME_SECRETAREAS_OF", text, sizeof(text) );
	cgi_R_Font_DrawString(x+w,y, va("%d %s %d",
										cg_entities[0].gent->client->sess.missionStats.enemiesKilled,
										text,
										cg_entities[0].gent->client->sess.missionStats.enemiesSpawned
										), 
							colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);
	*/

	y +=pad;
	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_FAVORITEWEAPON", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	
	int wpn=0,i;
	int max_wpn = cg_entities[0].gent->client->sess.missionStats.weaponUsed[0];
	for (i = 1; i<WP_NUM_WEAPONS; i++)
	{
		if (cg_entities[0].gent->client->sess.missionStats.weaponUsed[i] > max_wpn)
		{
			max_wpn = cg_entities[0].gent->client->sess.missionStats.weaponUsed[i];
			wpn = i;
		}
	}

	if ( wpn )
	{
		gitem_t	*wItem= FindItemForWeapon( (weapon_t)wpn);
		cgi_SP_GetStringTextString( va("INGAME_%s",wItem->classname ), text, sizeof( text ));
	//	cgi_R_Font_DrawString(x+w, y, va("%d",wpn), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);
		cgi_R_Font_DrawString(x+w, y, text, colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);
	}

	x = 334+70;
	y = 86;
	cgi_SP_GetStringTextString( "INGAME_SHOTSFIRED", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.shotsFired), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);


	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_HITS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.hits), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);


	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_ACCURACY", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	const float percent = cg_entities[0].gent->client->sess.missionStats.shotsFired? 100.0f * (float)cg_entities[0].gent->client->sess.missionStats.hits / cg_entities[0].gent->client->sess.missionStats.shotsFired : 0;
	cgi_R_Font_DrawString(x+w, y, va("%.2f%%",percent), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	if ( cg_entities[0].gent->client->sess.missionStats.weaponUsed[WP_SABER] <= 0 )
	{
		return; //don't have saber yet, so don't print any stats
	}
//first column, FORCE POWERS
	y =180;
	cgi_SP_GetStringTextString( "INGAME_FORCEUSE", text, sizeof(text) );
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_HEAL", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_HEAL]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_SPEED", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_SPEED]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_PULL", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_PULL]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_PUSH", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_PUSH]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString("INGAME_MINDTRICK", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_TELEPATHY]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_GRIP", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_GRIP]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_LIGHTNING", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.forceUsed[FP_LIGHTNING]), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

//second column, LIGHT SABER
	y = 180;
	x = 140;
	cgi_SP_GetStringTextString( "INGAME_LIGHTSABERUSE", text, sizeof(text) );
	cgi_R_Font_DrawString(x, y, text, colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_THROWN", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.saberThrownCnt), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_BLOCKS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.saberBlocksCnt), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_LEGATTACKS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.legAttacksCnt), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_ARMATTACKS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.armAttacksCnt), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_BODYATTACKS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.torsoAttacksCnt), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);

	y +=pad;
	cgi_SP_GetStringTextString( "INGAME_OTHERATTACKS", text, sizeof(text) );
w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 0.8f);	
	cgi_R_Font_DrawString(x,   y, text, colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 0.8f);
	cgi_R_Font_DrawString(x+w, y, va("%d",cg_entities[0].gent->client->sess.missionStats.otherAttacksCnt), colorTable[CT_WHITE], cgs.media.qhFontSmall, -1, 0.8f);
}


/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
return value is bool to NOT draw centerstring
=================
*/
qboolean CG_DrawScoreboard( void ) 
{
	// don't draw anything if the menu is up
	if ( cg_paused.integer ) 
	{
		return qfalse;
	}

	// Character is either dead, or a script has brought up the screen
	if (((cg.predicted_player_state.pm_type == PM_DEAD) && (cg.missionStatusDeadTime < level.time)) 
		|| (cg.missionStatusShow))
	{
		CG_MissionFailed();
		return qtrue;
	}

	return qfalse;
}

void ScoreBoardReset(void)
{
}

//================================================================================

