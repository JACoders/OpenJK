// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"
#include "cg_media.h"
#include "..\game\objectives.h"
#include "..\game\b_local.h"
#include "..\ui\ui_shared.h"

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
	char *text;

	if (!cg.missionFailedScreen)
	{ 
		cgi_UI_SetActive_Menu("missionfailed_menu");

		// If you're in camera mode when when you lose (Chewie kills you)
		// then the above fails. This should check for that. We'll end up
		// waiting until the camera stuff stops:
		if( !Menu_GetFocused() )
			return;

		cg.missionFailedScreen = qtrue;

		switch (statusTextIndex)
		{
			case -1:	//Our HERO DIED!!!
				text = "@SP_INGAME_MISSIONFAILED_PLAYER";
				break;
			case MISSIONFAILED_JAN:
				text = "@SP_INGAME_MISSIONFAILED_JAN";
					break;
			case MISSIONFAILED_LUKE:
				text = "@SP_INGAME_MISSIONFAILED_LUKE";
					break;
			case MISSIONFAILED_LANDO:
				text = "@SP_INGAME_MISSIONFAILED_LANDO";
					break;
			case MISSIONFAILED_R5D2:
				text = "@SP_INGAME_MISSIONFAILED_R5D2";
					break;
			case MISSIONFAILED_WARDEN:
				text = "@SP_INGAME_MISSIONFAILED_WARDEN";
					break;
			case MISSIONFAILED_PRISONERS:
				text = "@SP_INGAME_MISSIONFAILED_PRISONERS";
					break;
			case MISSIONFAILED_EMPLACEDGUNS:
				text = "@SP_INGAME_MISSIONFAILED_EMPLACEDGUNS";
					break;
			case MISSIONFAILED_LADYLUCK:
				text = "@SP_INGAME_MISSIONFAILED_LADYLUCK";
					break;
			case MISSIONFAILED_KYLECAPTURE:
				text = "@SP_INGAME_MISSIONFAILED_KYLECAPTURE";
					break;
			case MISSIONFAILED_TOOMANYALLIESDIED:
				text = "@SP_INGAME_MISSIONFAILED_TOOMANYALLIESDIED";
					break;

			case MISSIONFAILED_CHEWIE:
				text = "@SP_INGAME_MISSIONFAILED_CHEWIE";
					break;

			case MISSIONFAILED_KYLE:
				text = "@SP_INGAME_MISSIONFAILED_KYLE";
					break;

			case MISSIONFAILED_ROSH:
				text = "@SP_INGAME_MISSIONFAILED_ROSH";
					break;

			case MISSIONFAILED_WEDGE:
				text = "@SP_INGAME_MISSIONFAILED_WEDGE";
					break;

			case MISSIONFAILED_TURNED:
				text = "@SP_INGAME_MISSIONFAILED_TURNED";
					break;

			default:
				text = "@SP_INGAME_MISSIONFAILED_UNKNOWN";
					break;
		}
		//done with the variable for this time so reset it.
		statusTextIndex = -1;
		gi.cvar_set("ui_missionfailed_text", text);
	}
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

