// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"
#include "cg_media.h"
#include "..\game\objectives.h"


// For printing objectives
#ifdef _XBOX
static const short objectiveStartingYpos = 100;		// Y starting position for objective text
static const short objectiveStartingXpos = 130;		// X starting position for objective text
static const int objectiveTextBoxWidth = 400;		// Width (in pixels) of text box
static const int objectiveTextBoxHeight = 310;		// Height (in pixels) of text box
#else
static const short objectiveStartingYpos = 75;		// Y starting position for objective text
static const short objectiveStartingXpos = 60;		// X starting position for objective text
static const int objectiveTextBoxWidth = 500;		// Width (in pixels) of text box
static const int objectiveTextBoxHeight = 300;		// Height (in pixels) of text box
#endif // _XBOX

const char *showLoadPowersName[] = 
{
	"SP_INGAME_HEAL2",
	"SP_INGAME_JUMP2",
	"SP_INGAME_SPEED2",
	"SP_INGAME_PUSH2",
	"SP_INGAME_PULL2",
	"SP_INGAME_MINDTRICK2",
	"SP_INGAME_GRIP2",
	"SP_INGAME_LIGHTNING2",
	"SP_INGAME_SABER_THROW2",
	"SP_INGAME_SABER_OFFENSE2",
	"SP_INGAME_SABER_DEFENSE2",
	NULL,
};

#define MAX_OBJ_GRAPHICS 4
#define OBJ_GRAPHIC_SIZE 240
int obj_graphics[MAX_OBJ_GRAPHICS];

qboolean CG_ForcePower_Valid(int forceKnownBits, int index);

/*
====================
ObjectivePrint_Line

Print a single mission objective
====================
*/
static void ObjectivePrint_Line(const int color, const int objectIndex, int &missionYcnt)
{
	char *str,*strBegin;
	int y,pixelLen,charLen,i;
	const int maxHoldText = 1024;
	char holdText[maxHoldText];
	char finalText[2048];
	qhandle_t	graphic;

	int iYPixelsPerLine = cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f);

	cgi_SP_GetStringTextString( va("OBJECTIVES_%s",objectiveTable[objectIndex].name) , finalText, sizeof(finalText) );

	// A hack to be able to count prisoners 
	if (objectIndex==T2_RANCOR_OBJ5)
	{
		char value[64];
		int	currTotal, minTotal;

		gi.Cvar_VariableStringBuffer("ui_prisonerobj_currtotal",value,sizeof(value));
		currTotal = atoi(value);
		gi.Cvar_VariableStringBuffer("ui_prisonerobj_maxtotal",value,sizeof(value));
		minTotal = atoi(value);

		sprintf(finalText,va(finalText,currTotal,minTotal));
	}

	pixelLen = cgi_R_Font_StrLenPixels(finalText, cgs.media.qhFontMedium, 1.0f);

	str = finalText;

	if (cgi_Language_IsAsian())
	{
		// this is execrable, and should NOT have had to've been done now, but...
		//
		extern const char *CG_DisplayBoxedText(	int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight, 
												const char *psText, int iFontHandle, float fScale,
												const vec4_t v4Color);
		extern int giLinesOutput;
		extern float gfAdvanceHack;

		gfAdvanceHack = 1.0f;	// override internal vertical advance
		y = objectiveStartingYpos + (iYPixelsPerLine * missionYcnt);

		// Advance line if a graphic has printed
		for (i=0;i<MAX_OBJ_GRAPHICS;i++)
		{
			if (obj_graphics[i])
			{
				y += OBJ_GRAPHIC_SIZE + 4;
			}
		}

		CG_DisplayBoxedText(
			objectiveStartingXpos,
			y,
			objectiveTextBoxWidth,
			objectiveTextBoxHeight,
			finalText,	// int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight, const char *psText
			cgs.media.qhFontMedium,		// int iFontHandle, 
			1.0f,						// float fScale,
			colorTable[color]			// const vec4_t v4Color
			);

		gfAdvanceHack = 0.0f;	// restore
		missionYcnt += giLinesOutput;
	}
	else
	{
		// western...
		//
		if (pixelLen < objectiveTextBoxWidth)	// One shot - small enough to print entirely on one line
		{
			y =objectiveStartingYpos + (iYPixelsPerLine * (missionYcnt));

			cgi_R_Font_DrawString (
				objectiveStartingXpos, 
				y, 
				str, 
				colorTable[color], 
				cgs.media.qhFontMedium, 
				-1, 
				1.0f);

			++missionYcnt;
		}
		// Text is too long, break into lines.
		else
		{
			char holdText2[2];
			pixelLen = 0;
			charLen = 0;
			holdText2[1] = NULL;
			strBegin = str;

			while( *str ) 
			{
				holdText2[0] = *str;
				pixelLen += cgi_R_Font_StrLenPixels(holdText2, cgs.media.qhFontMedium, 1.0f);

				pixelLen += 2; // For kerning
				++charLen;

				if (pixelLen > objectiveTextBoxWidth ) 
				{	//Reached max length of this line
					//step back until we find a space
					while ((charLen>10) && (*str != ' ' ))
					{
						--str;
						--charLen;
					}

					if (*str==' ')
					{
						++str;	// To get past space
					}

					assert( charLen<maxHoldText );	// Too big?

					Q_strncpyz( holdText, strBegin, charLen);
					holdText[charLen] = NULL;
					strBegin = str;
					pixelLen = 0;
					charLen = 1;

					y = objectiveStartingYpos + (iYPixelsPerLine * missionYcnt);

					CG_DrawProportionalString(
						objectiveStartingXpos, 
						y, 
						holdText, 
						CG_SMALLFONT, 
						colorTable[color] );

					++missionYcnt;
				} 
				else if (*(str+1) == NULL)
				{
					++charLen;

					assert( charLen<maxHoldText );	// Too big?

					y = objectiveStartingYpos + (iYPixelsPerLine * missionYcnt);

					Q_strncpyz( holdText, strBegin, charLen);
					CG_DrawProportionalString(
						objectiveStartingXpos, 
						y, holdText, 
						CG_SMALLFONT, 
						colorTable[color] );

					++missionYcnt;
					break;
				}
				++str; 


			} 
		}
	}

	if (objectIndex == T3_BOUNTY_OBJ1)
	{
		y =objectiveStartingYpos + (iYPixelsPerLine * missionYcnt);
		if (obj_graphics[1])
		{
			y += OBJ_GRAPHIC_SIZE + 4;
		}
		if (obj_graphics[2])
		{
			y += OBJ_GRAPHIC_SIZE + 4;
		}
		graphic = cgi_R_RegisterShaderNoMip("textures/system/viewscreen1");
		CG_DrawPic( 105, 155, OBJ_GRAPHIC_SIZE * 0.90, OBJ_GRAPHIC_SIZE * 0.90, graphic );
		obj_graphics[3] = qtrue;
	}

}

/*
====================
CG_DrawDataPadObjectives

Draw routine for the objective info screen of the data pad.
====================
*/
void CG_DrawDataPadObjectives(const centity_t *cent )
{
	int		i,totalY;
	int		iYPixelsPerLine = cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f);

	const short titleXPos = objectiveStartingXpos - 22;		// X starting position for title text
	const short titleYPos = objectiveStartingYpos - 23;		// Y starting position for title text
	const short graphic_size = 16;							// Size (width and height) of graphic used to show status of objective
	const short graphicXpos = objectiveStartingXpos - graphic_size - 8;	// Amount of X to backup from text starting position
	const short graphicYOffset = (iYPixelsPerLine - graphic_size)/2;	// Amount of Y to raise graphic so it's in the center of the text line

	missionInfo_Updated = qfalse;		// This will stop the text from flashing
	cg.missionInfoFlashTime = 0;

	// zero out objective graphics 
	for (i=0;i<MAX_OBJ_GRAPHICS;i++)
	{
		obj_graphics[i] = qfalse;
	}

	// Title Text at the top 
	char text[1024]={0};
	cgi_SP_GetStringTextString( "SP_INGAME_OBJECTIVES", text, sizeof(text) );
	cgi_R_Font_DrawString (titleXPos, titleYPos, text, colorTable[CT_TITLE], cgs.media.qhFontMedium, -1, 1.0f);

	int missionYcnt = 0;

	// Print all active objectives
	for (i=0;i<MAX_OBJECTIVES;i++)
	{
		// Is there an objective to see?
		if (cent->gent->client->sess.mission_objectives[i].display)
		{
			// Calculate the Y position
			totalY = objectiveStartingYpos + (iYPixelsPerLine * (missionYcnt))+(iYPixelsPerLine/2);

			//	Draw graphics that show if mission has been accomplished or not
			cgi_R_SetColor(colorTable[CT_BLUE3]);
			CG_DrawPic( (graphicXpos),   (totalY-graphicYOffset),   graphic_size,  graphic_size, cgs.media.messageObjCircle);	// Circle in front
			if (cent->gent->client->sess.mission_objectives[i].status == OBJECTIVE_STAT_SUCCEEDED)
			{
				CG_DrawPic( (graphicXpos),   (totalY-graphicYOffset),   graphic_size,  graphic_size, cgs.media.messageLitOn);	// Center Dot
			}

			// Print current objective text
			ObjectivePrint_Line(CT_WHITE, i, missionYcnt );
		}
	}

	// No mission text?
	if (!missionYcnt)
	{
		// Set the message a quarter of the way down and in the center of the text box
		int messageYPosition = objectiveStartingYpos + (objectiveTextBoxHeight / 4);

		cgi_SP_GetStringTextString( "SP_INGAME_OBJNONE", text, sizeof(text) );
		int messageXPosition = objectiveStartingXpos + (objectiveTextBoxWidth/2) -  (cgi_R_Font_StrLenPixels(text, cgs.media.qhFontMedium, 1.0f) /2);

		cgi_R_Font_DrawString (
			messageXPosition, 
			messageYPosition, 
			text, 
			colorTable[CT_WHITE], 
			cgs.media.qhFontMedium, 
			-1, 
			1.0f);
	}
}

/*

//-------------------------------------------------------
static void CG_DrawForceCount( const int force, int x, float *y, const int pad,qboolean *hasForcePowers )
{
	char	s[MAX_STRING_CHARS];
	int		val, textColor;
	char	text[1024]={0};

	gi.Cvar_VariableStringBuffer( va("playerfplvl%d", force ),s, sizeof(s) );

	sscanf( s, "%d",&val );

	if ((val<1) || (val> NUM_FORCE_POWERS))	
	{
		return;
	}

	textColor = CT_ICON_BLUE;

	// Draw title
	cgi_SP_GetStringTextString( showLoadPowersName[force], text, sizeof(text) );
	CG_DrawProportionalString( x, *y, text, CG_BIGFONT, colorTable[textColor] );


	// Draw icons
	cgi_R_SetColor( colorTable[CT_WHITE]);
	const int iconSize = 30;
	if ( val >= 0 )
	{
		x -= 10;	// Back up from title a little

		for ( int i = 0; i < val; i++ )
		{
			CG_DrawPic( x - iconSize - i * (iconSize + 10) , *y, iconSize, iconSize, force_icons[force] ); 
		}
	}

	*y += pad;

	*hasForcePowers = qtrue;
}


/*
====================
CG_LoadScreen_PersonalInfo
====================
*/
/*
static void CG_LoadScreen_PersonalInfo(void)
{
	float	x, y;
	int		pad = 25;
	char	text[1024]={0};
	qboolean	hasForcePowers;

	y = 65 + 30;

	pad = 28;
	x = 300;
	hasForcePowers=qfalse;

	CG_DrawForceCount( FP_HEAL, x, &y, pad,&hasForcePowers);
	CG_DrawForceCount( FP_LEVITATION, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_SPEED, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_PUSH, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_PULL, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_TELEPATHY, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_GRIP, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_LIGHTNING, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_SABERTHROW, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_SABER_OFFENSE, x, &y, pad,&hasForcePowers );
	CG_DrawForceCount( FP_SABER_DEFENSE, x, &y, pad,&hasForcePowers );
	
	if (hasForcePowers)
	{	
		cgi_SP_GetStringTextString( "SP_INGAME_CURRENTFORCEPOWERS", text, sizeof(text) );
		CG_DrawProportionalString( 200, 65, text, CG_CENTER | CG_BIGFONT, colorTable[CT_WHITE] );
	}
	else
	{	//you are only totally empty on the very first map?
//		cgi_SP_GetStringTextString( "SP_INGAME_NONE", text, sizeof(text) );
//		CG_DrawProportionalString( 320, y+30, text, CG_CENTER | CG_BIGFONT, colorTable[CT_ICON_BLUE] );
		cgi_SP_GetStringTextString( "SP_INGAME_ALONGTIME", text, sizeof(text) );
		int w = cgi_R_Font_StrLenPixels(text,cgs.media.qhFontMedium, 1.5f);
		cgi_R_Font_DrawString((320)-(w/2), y+40, text,  colorTable[CT_ICON_BLUE], cgs.media.qhFontMedium, -1, 1.5f);
	}

}
*/

static void CG_LoadBar(void)
{
	cgi_R_SetColor( colorTable[CT_WHITE]);

	int glowHeight = (cg.loadLCARSStage / 9.0f) * 147;
	int glowTop = (280 + 147) - glowHeight;

	// Draw glow:
	CG_DrawPic(280, glowTop, 73, glowHeight, cgs.media.loadTick);

	// Draw saber:
	CG_DrawPic(280, 265, 73, 147, cgs.media.levelLoad);
}

int CG_WeaponCheck( int weaponIndex );


int			loadForcePowerLevel[NUM_FORCE_POWERS];

/*
===============
ForcePowerDataPad_Valid
===============
*/
qboolean CG_ForcePower_Valid(int forceKnownBits, int index)
{
	if ((forceKnownBits & (1 << showPowers[index])) && 
		loadForcePowerLevel[showPowers[index]])	// Does he have the force power?
	{
		return qtrue;
	}

	return qfalse;
}

// Get the player weapons and force power info 
static void CG_GetLoadScreenInfo(int *weaponBits,int *forceBits)
{
	char	s[MAX_STRING_CHARS];
	int		iDummy,i;
	float	fDummy;
	const char	*var;

	gi.Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );

	// Get player weapons and force powers known
	if (s[0])	
	{
	//				|general info				  |-force powers 
		sscanf( s, "%i %i %i %i %i %i %i %f %f %f %i %i", 
				&iDummy,	//	&client->ps.stats[STAT_HEALTH],
				&iDummy,	//	&client->ps.stats[STAT_ARMOR],
				&*weaponBits,//	&client->ps.stats[STAT_WEAPONS],
				&iDummy,	//	&client->ps.stats[STAT_ITEMS],
				&iDummy,	//	&client->ps.weapon,
				&iDummy,	//	&client->ps.weaponstate,
				&iDummy,	//	&client->ps.batteryCharge,
				&fDummy,	//	&client->ps.viewangles[0],
				&fDummy,	//	&client->ps.viewangles[1],
				&fDummy,	//	&client->ps.viewangles[2],
							//force power data
				&*forceBits,	//	&client->ps.forcePowersKnown,
				&iDummy		//	&client->ps.forcePower,

				);
	}

	// the new JK2 stuff - force powers, etc...
	//
	gi.Cvar_VariableStringBuffer( "playerfplvl", s, sizeof(s) );
	i=0;
	var = strtok( s, " " );
	while( var != NULL )
	{
		/* While there are tokens in "s" */
		loadForcePowerLevel[i++] = atoi(var);
		/* Get next token: */
		var = strtok( NULL, " " );
	}

}

/*
====================
CG_DrawLoadingScreen

Load screen displays the map pic, the mission briefing and weapons/force powers
====================
*/
static void CG_DrawLoadingScreen( qhandle_t	levelshot, qhandle_t levelshot2, const char *mapName) 
{
	int xPos,yPos,width,height;
	vec4_t	color;
	qhandle_t	background;
	int weapons=0, forcepowers=0;

	// Get mission briefing for load screen
	if (cgi_SP_GetStringTextString( va("BRIEFINGS_%s",mapName), NULL, 0 ) == 0)
	{
		cgi_Cvar_Set( "ui_missionbriefing", "@BRIEFINGS_NONE" );
	}
	else
	{
		cgi_Cvar_Set( "ui_missionbriefing", va("@BRIEFINGS_%s",mapName) );
	}

	// Print background
	if (cgi_UI_GetMenuItemInfo(
		"loadScreen",
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

	// Print level pic
	if (cgi_UI_GetMenuItemInfo(
		"loadScreen",
		"mappic",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, levelshot );
	}

	// Print second level pic
	if (cgi_UI_GetMenuItemInfo(
		"loadScreen",
		"mappic2",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor( color );
		CG_DrawPic( xPos, yPos, width, height, levelshot2 );
	}

	// Removed by BTO - No more icons on the loading screen
/*
	// Get player weapons and force power info
	CG_GetLoadScreenInfo(&weapons,&forcepowers);

	// Print weapon icons
	if (weapons)
	{
		CG_DrawLoadWeapons(weapons);
	}

	// Print force power icons
	if (forcepowers)
	{
		CG_DrawLoadForcePowers(forcepowers);
	}
*/
}

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	int		y;

	// draw the dialog background
	const char	*info	= CG_ConfigString( CS_SERVERINFO );
	const char	*s		= Info_ValueForKey( info, "mapname" );

	extern SavedGameJustLoaded_e g_eSavedGameJustLoaded;	// hack! (hey, it's the last week of coding, ok?

	if ( g_eSavedGameJustLoaded != eFULL && (!strcmp(s,"yavin1") || !strcmp(s,"demo")) )//special case for first map!
	{
		char	text[1024]={0};

		//
		cgi_R_SetColor( colorTable[CT_BLACK] );
		CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, cgs.media.whiteShader );

		cgi_SP_GetStringTextString( "SP_INGAME_ALONGTIME", text, sizeof(text) );

		int w = cgi_R_Font_StrLenPixels(text,cgs.media.qhFontMedium, 1.0f);
		cgi_R_Font_DrawString((320)-(w/2), 140, text,  colorTable[CT_ICON_BLUE], cgs.media.qhFontMedium, -1, 1.0f);
	}
	else
	{
		qhandle_t levelshot = cgi_R_RegisterShaderNoMip( va( "levelshots/%s", s ) );
		if (!levelshot) {
			levelshot = cgi_R_RegisterShaderNoMip( "menu/art/unknownmap" );	
		}

		qhandle_t levelshot2 = cgi_R_RegisterShaderNoMip( va( "levelshots/%s2", s ) );
		if (!levelshot2) {
			levelshot2 = levelshot;
		}

		CG_DrawLoadingScreen(levelshot, levelshot2, s);
		cgi_UI_MenuPaintAll();
	}

	CG_LoadBar();


	// the first 150 rows are reserved for the client connection
	// screen to write into
//	if ( cg.processedSnapshotNum == 0 ) 
	{
		// still loading
		// print the current item being loaded

#ifdef _DEBUG
		cgi_R_Font_DrawString( 40, 416, va("LOADING ... %s",cg.infoScreenText),colorTable[CT_LTGOLD1], cgs.media.qhFontSmall, -1, 1.0f );
#endif
	}

	// draw info string information

	y = 20;
	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );

	if ( s[0] ) 
	{
		if (s[0] == '@')
		{	
			char text[1024]={0};
			cgi_SP_GetStringTextString( s+1, text, sizeof(text) );
			cgi_R_Font_DrawString( 15, y, va("\"%s\"",text),colorTable[CT_WHITE],cgs.media.qhFontMedium, -1, 1.0f );
		}
		else 
		{
			cgi_R_Font_DrawString( 15, y, va("\"%s\"",s),colorTable[CT_WHITE],cgs.media.qhFontMedium, -1, 1.0f );
		}
		y += 20;
	}
}