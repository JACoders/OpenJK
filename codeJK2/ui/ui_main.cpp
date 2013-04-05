// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

// leave this at the top of all UI_xxxx files for PCH reasons...
//
#include "../server/exe_headers.h"

#include "ui_local.h"

#include "menudef.h"

#include "ui_shared.h"

#include "../qcommon/stv_version.h"

#define LISTBUFSIZE 10240

static struct 
{
	char	listBuf[LISTBUFSIZE];			//	The list of file names read in

	// For scrolling through file names 
	int				currentLine;		//	Index to currentSaveFileComments[] currently highlighted
	int				saveFileCnt;		//	Number of save files read in

	int				awaitingSave;		//	Flag to see if user wants to overwrite a game.

	char			*savegameMap;
	int				savegameFromFlag;
} s_savegame;

#define MAX_SAVELOADFILES	100
#define MAX_SAVELOADNAME	32

byte screenShotBuf[SG_SCR_WIDTH * SG_SCR_HEIGHT * 4];

typedef struct 
{
	char *currentSaveFileName;						// file name of savegame
	char currentSaveFileComments[iSG_COMMENT_SIZE];	// file comment
	char currentSaveFileDateTimeString[iSG_COMMENT_SIZE];	// file time and date
	time_t currentSaveFileDateTime;
	char currentSaveFileMap[MAX_TOKEN_CHARS];			// map save game is from
} savedata_t;

static savedata_t s_savedata[MAX_SAVELOADFILES];
void UI_SetActiveMenu( const char* menuname,const char *menuID );
void ReadSaveDirectory (void);

qboolean		Asset_Parse(char **buffer);
menuDef_t		*Menus_FindByName(const char *p);
void			Menus_HideItems(const char *menuName);
int				Text_Height(const char *text, float scale, int iFontIndex );
int				Text_Width(const char *text, float scale, int iFontIndex );
void			_UI_DrawTopBottom(float x, float y, float w, float h, float size);
void			_UI_DrawSides(float x, float y, float w, float h, float size);
void			UI_CheckVid1Data(const char *menuTo,const char *warningMenuName);
void			UI_GetVideoSetup ( void );
void			UI_UpdateVideoSetup ( void );
void			UI_LoadMenus(const char *menuFile, qboolean reset);
static void		UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int iFontIndex);
static qboolean UI_OwnerDrawVisible(int flags);
int				UI_OwnerDrawWidth(int ownerDraw, float scale);
static void		UI_Update(const char *name);
void			UI_UpdateCvars( void );
void			UI_ResetDefaults( void );


static int gamecodetoui[] = {4,2,3,0,5,1,6};

uiInfo_t uiInfo;

static void UI_RegisterCvars( void );
void UI_Load(void);


typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;


vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;
vmCvar_t	ui_menuFiles;
vmCvar_t	ui_smallFont;
vmCvar_t	ui_bigFont;
vmCvar_t	ui_hudFiles;

cvarTable_t		cvarTable[] = 
{
	{ &ui_drawCrosshair,		"cg_drawCrosshair",		"1",			CVAR_ARCHIVE },
	{ &ui_marks,				"cg_marks",				"1",			CVAR_ARCHIVE },

	{ &ui_menuFiles,			"ui_menuFiles",			"ui/menus.txt", CVAR_ARCHIVE },
	{ &ui_smallFont,			"ui_smallFont",			"0.25",			CVAR_ARCHIVE},
	{ &ui_bigFont,				"ui_bigFont",			"0.4",			CVAR_ARCHIVE},
	{ &ui_hudFiles,				"cg_hudFiles",			"ui/jk2hud.txt",CVAR_ARCHIVE}, 
};

int		cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);

void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, int iMaxPixelWidth, int style, int iFontIndex);
int Key_GetCatcher( void );

#define	UI_FPS_FRAMES	4
void _UI_Refresh( int realtime )
{
	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	if ( !( Key_GetCatcher() & KEYCATCH_UI ) ) 
	{
		return;
	}

	extern void SP_CheckForLanguageUpdates(void);
	SP_CheckForLanguageUpdates();

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) 
	{
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) 
		{
			total += previousTimes[i];
		}
		if ( !total ) 
		{
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}



	UI_UpdateCvars();

	if (Menu_Count() > 0) 
	{
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
//		UI_DoServerRefresh();
		// refresh server status
//		UI_BuildServerStatus(qfalse);
		// refresh find player list
//		UI_BuildFindPlayerList(qfalse);
	} 
	
	// draw cursor
	UI_SetColor( NULL );
	if (Menu_Count() > 0)
	{
		if (uiInfo.uiDC.cursorShow == qtrue)
		{
			UI_DrawHandlePic( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 48, 48, uiInfo.uiDC.Assets.cursor);
		}
	}
}

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods() {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	uiInfo.modCount = 0;
	numdirs = FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName = String_Alloc(dirptr);
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc(descptr);
		dirptr += dirlen + strlen(descptr) + 1;
		uiInfo.modCount++;
		if (uiInfo.modCount >= MAX_MODS) {
			break;
		}
	}

}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) 
{
	return 0;
}



/*
================
Text_PaintChar
================
*/
/*
static void Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) 
{
	float w, h;

	w = width * scale;
	h = height * scale;
	ui.R_DrawStretchPic((int)x, (int)y, w, h, s, t, s2, t2, hShader );	//make the coords (int) or else the chars bleed
}
*/

/*
================
Text_Paint
================
*/
// iMaxPixelWidth is 0 here for no limit (but gets converted to -1), else max printable pixel width relative to start pos
//
void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, int iMaxPixelWidth, int style, int iFontIndex)
{
	if (iFontIndex == 0)
	{
		iFontIndex = uiInfo.uiDC.Assets.qhMediumFont;
	}
	// kludge.. convert JK2 menu styles to SOF2 printstring ctrl codes...
	//
	int iStyleOR = 0;
	switch (style)
	{
//	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
//	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	}

	ui.R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!iMaxPixelWidth?-1:iMaxPixelWidth,	// iMaxPixelWidth (-1 = none)
							scale	// const float scale = 1.0f
							);
}


/*
================
Text_PaintWithCursor
================
*/
// iMaxPixelWidth is 0 here for no-limit
void Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int iMaxPixelWidth, int style, int iFontIndex) 
{
	Text_Paint(x, y, scale, color, text, iMaxPixelWidth, style, iFontIndex);

	// now print the cursor as well...
	//
	char sTemp[1024];
	int iCopyCount = min(strlen(text), cursorPos);
		iCopyCount = min(iCopyCount,sizeof(sTemp));

	// copy text into temp buffer for pixel measure...
	//
	strncpy(sTemp,text,iCopyCount);
			sTemp[iCopyCount] = '\0';
	
	int iNextXpos  = ui.R_Font_StrLenPixels(sTemp, iFontIndex, scale );

	Text_Paint(x+iNextXpos, y, scale, color, va("%c",cursor), iMaxPixelWidth, style|ITEM_TEXTSTYLE_BLINK, iFontIndex);
}


const char *UI_FeederItemText(float feederID, int index, int column, qhandle_t *handle) 
{
	*handle = -1;

	if (feederID == FEEDER_SAVEGAMES) 
	{
		if (column==0)
		{
			return s_savedata[index].currentSaveFileComments;
		}
		else
		{
			return s_savedata[index].currentSaveFileDateTimeString;
		}
	}
/*	if (feederID == FEEDER_MAPS) 
	{
		int actual;
		return UI_SelectedMap(index, &actual);
	} 
	else 
*/	if (feederID == FEEDER_MODS) 
	{
		if (index >= 0 && index < uiInfo.modCount) 
		{
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr) 
			{
				return uiInfo.modList[index].modDescr;
			} 
			else 
			{
				return uiInfo.modList[index].modName;
			}
		}
	} 
/*
	else if (feederID == FEEDER_DEMOS) 
	{
		if (index >= 0 && index < uiInfo.demoCount) 
		{
			return uiInfo.demoList[index];
		}
	}
*/
	return "";
}

qhandle_t UI_FeederItemImage(float feederID, int index) 
{
	if (feederID == FEEDER_SAVEGAMES) 
	{
		/*
		if (index >= 0 && index < uiInfo.characterCount) 
		{
			if (uiInfo.characterList[index].headImage == -1) 
			{
				uiInfo.characterList[index].headImage = trap_R_RegisterShaderNoMip(uiInfo.characterList[index].imageName);
			}

			return uiInfo.characterList[index].headImage;
		}*/
	} 
/*	else if (feederID == FEEDER_Q3HEADS) 
	{
		if (index >= 0 && index < uiInfo.q3HeadCount) 
		{
			return uiInfo.q3HeadIcons[index];
		}
	} 
	else if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS) 
	{
		int actual;
		UI_SelectedMap(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.mapCount) 
		{
			if (uiInfo.mapList[index].levelShot == -1) 
			{
				uiInfo.mapList[index].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}
	*/
	return 0;
}


/*
=================
CreateNextSaveName
=================
*/
static int CreateNextSaveName(char *fileName)
{
	int i;

	// Loop through all the save games and look for the first open name
	for (i=0;i<MAX_SAVELOADFILES;i++)
	{
		Com_sprintf( fileName, MAX_SAVELOADNAME, "jkii%02d", i );

		if (!ui.SG_GetSaveGameComment(fileName, NULL, NULL))
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
===============
UI_DeferMenuScript

Return true if the menu script should be deferred for later
===============
*/
static qboolean UI_DeferMenuScript ( const char **args )
{
	const char* name;

	// Whats the reason for being deferred?
	if (!String_Parse(args, &name)) 
	{
		return qfalse;
	}

	// Handle the custom cases
	if ( !Q_stricmp ( name, "VideoSetup" ) )
	{
		const char* warningMenuName;
		qboolean	deferred;

		// No warning menu specified
		if ( !String_Parse(args, &warningMenuName) )
		{
			return qfalse;
		}

		// Defer if the video options were modified
		deferred = Cvar_VariableIntegerValue( "ui_r_modified" ) ? qtrue : qfalse;

		if ( deferred )
		{
			// Open the warning menu
			Menus_OpenByName(warningMenuName);
		}

		return deferred;
	}

	return qfalse;
}

/*
===============
UI_RunMenuScript
===============
*/
static qboolean UI_RunMenuScript ( const char **args ) 
{
	const char *name, *name2,*mapName,*menuName,*warningMenuName;

	if (String_Parse(args, &name)) 
	{
		if (Q_stricmp(name, "resetdefaults") == 0)		
		{
			UI_ResetDefaults();
		}
		else if (Q_stricmp(name, "saveControls") == 0) 
		{
			Controls_SetConfig(qtrue);
		} 
		else if (Q_stricmp(name, "loadControls") == 0) 
		{
			Controls_GetConfig();
		} 
		else if (Q_stricmp(name, "clearError") == 0) 
		{
			Cvar_Set("com_errorMessage", "");
		} 
		else if (Q_stricmp(name, "ReadSaveDirectory") == 0) 
		{
			s_savegame.saveFileCnt = -1;	//force a refresh at drawtime
//			ReadSaveDirectory();
		} 
		else if (Q_stricmp(name, "loadAuto") == 0) 
		{
			Menus_CloseAll();
			ui.Cmd_ExecuteText( EXEC_APPEND, "load auto\n");
		}
		else if (Q_stricmp(name, "loadgame") == 0) 
		{
			if (s_savedata[s_savegame.currentLine].currentSaveFileName)// && (*s_file_desc_field.field.buffer))
			{
				Menus_CloseAll();
				ui.Cmd_ExecuteText( EXEC_APPEND, va("load %s\n", s_savedata[s_savegame.currentLine].currentSaveFileName));
			}
		}
		else if (Q_stricmp(name, "deletegame") == 0) 
		{
			if (s_savedata[s_savegame.currentLine].currentSaveFileName)	// A line was chosen
			{
				ui.Printf( va("%s\n","Attempting to delete game"));

				ui.Cmd_ExecuteText( EXEC_NOW, va("wipe %s\n", s_savedata[s_savegame.currentLine].currentSaveFileName));
//				ReadSaveDirectory();	//refresh
				s_savegame.saveFileCnt = -1;	//force a refresh at drawtime

			}
		}
		else if (Q_stricmp(name, "savegame") == 0) 
		{
			char fileName[MAX_SAVELOADNAME];
			char description[64];
			// Create a new save game
//			if ( !s_savedata[s_savegame.currentLine].currentSaveFileName)	// No line was chosen
			{
				CreateNextSaveName(fileName);	// Get a name to save to
			}
//			else	// Overwrite a current save game? Ask first.
			{
//				s_savegame.yes.generic.flags	= QMF_HIGHLIGHT_IF_FOCUS;
//				s_savegame.no.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;

//				strcpy(fileName,s_savedata[s_savegame.currentLine].currentSaveFileName);
//				s_savegame.awaitingSave = qtrue;
//				s_savegame.deletegame.generic.flags	= QMF_GRAYED;	// Turn off delete button
//				break;
			}

			// Save description line
			ui.Cvar_VariableStringBuffer("ui_gameDesc",description,sizeof(description));
			ui.SG_StoreSaveGameComment(description);

			ui.Cmd_ExecuteText( EXEC_APPEND, va("save %s\n", fileName));
			s_savegame.saveFileCnt = -1;	//force a refresh the next time around
		}
		else if (Q_stricmp(name, "LoadMods") == 0) 
		{
			UI_LoadMods();
		} 
		else if (Q_stricmp(name, "RunMod") == 0) 
		{
			Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName);
extern	void FS_Restart( void );
			FS_Restart();
			Cbuf_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} 
		// FIXME BOB - do we want this?
		/*
		else if (Q_stricmp(name, "RunDemo") == 0) 
		{
			Cbuf_ExecuteText( EXEC_APPEND, va("demo %s\n", uiInfo.demoList[uiInfo.demoIndex]));
		} 
		*/
		else if (Q_stricmp(name, "Quit") == 0) 
		{
			Cbuf_ExecuteText( EXEC_NOW, "quit");
		} 
		else if (Q_stricmp(name, "Controls") == 0) 
		{
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("setup_menu2");
		} 
		else if (Q_stricmp(name, "Leave") == 0) 
		{
			Cbuf_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("mainMenu");
		} 
		else if (Q_stricmp(name, "getvideosetup") == 0) 
		{
			UI_GetVideoSetup ( );
		}
		else if (Q_stricmp(name, "updatevideosetup") == 0)
		{
			UI_UpdateVideoSetup ( );
		}
		else if (Q_stricmp(name, "nextDataPadForcePower") == 0)		
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpforcenext\n");
		}
		else if (Q_stricmp(name, "prevDataPadForcePower") == 0)		
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpforceprev\n");
		}
		else if (Q_stricmp(name, "nextDataPadWeapon") == 0)		
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpweapnext\n");
		}
		else if (Q_stricmp(name, "prevDataPadWeapon") == 0)		
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpweapprev\n");
		}
		else if (Q_stricmp(name, "nextDataPadInventory") == 0)		
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpinvnext\n");
		}
		else if (Q_stricmp(name, "prevDataPadInventory") == 0)		
		{
			ui.Cmd_ExecuteText( EXEC_APPEND, "dpinvprev\n");
		}
		else if (Q_stricmp(name, "checkvid1data") == 0)		// Warn user data has changed before leaving screen?
		{
			String_Parse(args, &menuName);

			String_Parse(args, &warningMenuName);

			UI_CheckVid1Data(menuName,warningMenuName);
		}
		else if (Q_stricmp(name, "startgame") == 0) 
		{
			Menus_CloseAll();
			if ( Cvar_VariableIntegerValue("com_demo") )
			{
#ifdef FINAL_BUILD
				ui.Cmd_ExecuteText( EXEC_APPEND, "map demo\n");
#else
				ui.Cmd_ExecuteText( EXEC_APPEND, "devmap demo\n");
#endif
			}
			else
			{
#ifdef FINAL_BUILD
				ui.Cmd_ExecuteText( EXEC_APPEND, "map kejim_post\n");
#else
				ui.Cmd_ExecuteText( EXEC_APPEND, "devmap kejim_post\n");
#endif
			}
		} 
		else if (Q_stricmp(name, "startmap") == 0) 
		{
			Menus_CloseAll();

			String_Parse(args, &mapName);

			ui.Cmd_ExecuteText( EXEC_APPEND, va("map %s\n",mapName));
		} 
		else if (Q_stricmp(name, "closeingame") == 0) 
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
			Menus_ActivateByName("mainhud");
		} 
		else if (Q_stricmp(name, "closedatapad") == 0) 
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
			Menus_ActivateByName("mainhud");

			Cvar_Set( "cg_updatedDataPadForcePower1", "0" );
			Cvar_Set( "cg_updatedDataPadForcePower2", "0" );
			Cvar_Set( "cg_updatedDataPadForcePower3", "0" );
			Cvar_Set( "cg_updatedDataPadObjective", "0" );
		} 
		else if (Q_stricmp(name, "glCustom") == 0) 
		{
			Cvar_Set("ui_r_glCustom", "4");
		} 
		else if (Q_stricmp(name, "update") == 0) 
		{
			if (String_Parse(args, &name2)) 
			{
				UI_Update(name2);
			}
			else 
			{
				Com_Printf("update missing cmd\n");
			}
		}
		else 
		{
			Com_Printf("unknown UI script %s\n", name);
		}
	}

	return qtrue;
}

/*
=================
UI_GetValue
=================
*/
static float UI_GetValue(int ownerDraw) 
{
  return 0;
}

/*
=================
UI_StopCinematic
=================
*/
static void UI_StopCinematic(int handle) 
{
	if (handle >= 0) 
	{
		trap_CIN_StopCinematic(handle);
	} 
	else 
	{
		handle = abs(handle);
		if (handle == UI_MAPCINEMATIC) 
		{
			// FIXME - BOB do we need this?
//			if (uiInfo.mapList[ui_currentMap.integer].cinematic >= 0) 
//			{
//				trap_CIN_StopCinematic(uiInfo.mapList[ui_currentMap.integer].cinematic);
//				uiInfo.mapList[ui_currentMap.integer].cinematic = -1;
//			}
		} 
		else if (handle == UI_NETMAPCINEMATIC) 
		{
			// FIXME - BOB do we need this?
//			if (uiInfo.serverStatus.currentServerCinematic >= 0) 
//			{
//				trap_CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
//				uiInfo.serverStatus.currentServerCinematic = -1;
//			}
		} 
		else if (handle == UI_CLANCINEMATIC) 
		{
			// FIXME - BOB do we need this?
//			int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
//			if (i >= 0 && i < uiInfo.teamCount) 
//			{
//				if (uiInfo.teamList[i].cinematic >= 0) 
//				{
//					trap_CIN_StopCinematic(uiInfo.teamList[i].cinematic);
//					uiInfo.teamList[i].cinematic = -1;
//				}
//			}
		}
	}
}
static UI_HandleLoadSelection()
{
	Cvar_Set("ui_SelectionOK", va("%d",(s_savegame.currentLine < s_savegame.saveFileCnt)) );
	Cvar_Set("ui_gameDesc", s_savedata[s_savegame.currentLine].currentSaveFileComments );	// set comment 
	if (!ui.SG_GetSaveImage(s_savedata[s_savegame.currentLine].currentSaveFileName, &screenShotBuf))
	{
		memset( screenShotBuf,0,(SG_SCR_WIDTH * SG_SCR_HEIGHT * 4)); 
	}
}

/*
=================
UI_FeederCount
=================
*/
static int UI_FeederCount(float feederID) 
{
	if (feederID == FEEDER_SAVEGAMES) 
	{
		if (s_savegame.saveFileCnt == -1)
		{
			ReadSaveDirectory();	//refresh
			UI_HandleLoadSelection();
		}
		return s_savegame.saveFileCnt;
	} 
	else if (feederID == FEEDER_MODS) 
	{
		return uiInfo.modCount;
	} 
	else if (feederID == FEEDER_DEMOS) 
	{
		return uiInfo.demoCount;
	}
	return 0;
}

/*
=================
UI_FeederSelection
=================
*/
static void UI_FeederSelection(float feederID, int index) 
{
	static char info[MAX_STRING_CHARS];

	if (feederID == FEEDER_SAVEGAMES) 
	{
		s_savegame.currentLine = index;
		UI_HandleLoadSelection();
	} 
	else if (feederID == FEEDER_MODS) 
	{
		uiInfo.modIndex = index;
	} 
	else if (feederID == FEEDER_CINEMATICS) 
	{
		uiInfo.movieIndex = index;
		if (uiInfo.previewMovie >= 0) 
		{
			trap_CIN_StopCinematic(uiInfo.previewMovie);
		}
		uiInfo.previewMovie = -1;
	} 
	else if (feederID == FEEDER_DEMOS) 
	{
		uiInfo.demoIndex = index;
	}
}

void Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void Key_GetBindingBuf( int keynum, char *buf, int buflen );

static qboolean UI_Crosshair_HandleKey(int flags, float *special, int key) 
{
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
  {
		if (key == A_MOUSE2) 
		{
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair)); 
		return qtrue;
	}
	return qfalse;
}


static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) 
{

	switch (ownerDraw) 
	{
		case UI_CROSSHAIR:
			UI_Crosshair_HandleKey(flags, special, key);
			break;
		default:
			break;
	}

  return qfalse;
}


/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad ) 
{
	int start;

	UI_RegisterCvars();

	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0/480.0);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0/640.0);
	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) 
	{
		// wide screen
		uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else 
	{
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}

	Init_Display(&uiInfo.uiDC);

	uiInfo.uiDC.drawText			= &Text_Paint;
	uiInfo.uiDC.drawHandlePic		= &UI_DrawHandlePic;
	uiInfo.uiDC.drawRect			= &_UI_DrawRect;
	uiInfo.uiDC.drawSides			= &_UI_DrawSides;
	uiInfo.uiDC.drawTextWithCursor	= &Text_PaintWithCursor;
	uiInfo.uiDC.executeText			= &Cbuf_ExecuteText;
	uiInfo.uiDC.drawTopBottom		= &_UI_DrawTopBottom;
	uiInfo.uiDC.feederCount			= &UI_FeederCount;
	uiInfo.uiDC.feederSelection		= &UI_FeederSelection;
	uiInfo.uiDC.fillRect			= &UI_FillRect;
	uiInfo.uiDC.getBindingBuf		= &Key_GetBindingBuf;
	uiInfo.uiDC.getCVarString		= Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue		= trap_Cvar_VariableValue;
	uiInfo.uiDC.getOverstrikeMode	= &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.getValue			= &UI_GetValue;
	uiInfo.uiDC.keynumToStringBuf	= &Key_KeynumToStringBuf;
	uiInfo.uiDC.modelBounds			= &trap_R_ModelBounds;
	uiInfo.uiDC.ownerDrawVisible	= &UI_OwnerDrawVisible;
	uiInfo.uiDC.ownerDrawWidth		= &UI_OwnerDrawWidth;
	uiInfo.uiDC.ownerDrawItem		= &UI_OwnerDraw;
	uiInfo.uiDC.Print				= &Com_Printf; 
	uiInfo.uiDC.registerSound		= &trap_S_RegisterSound;
	uiInfo.uiDC.registerModel		= ui.R_RegisterModel;
	uiInfo.uiDC.clearScene			= &trap_R_ClearScene;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene			= &trap_R_RenderScene;
	uiInfo.uiDC.runScript			= &UI_RunMenuScript;
	uiInfo.uiDC.deferScript			= &UI_DeferMenuScript;
	uiInfo.uiDC.setBinding			= &trap_Key_SetBinding;
	uiInfo.uiDC.setColor			= &UI_SetColor;
	uiInfo.uiDC.setCVar				= Cvar_Set;
	uiInfo.uiDC.setOverstrikeMode	= &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.startLocalSound		= &trap_S_StartLocalSound;
	uiInfo.uiDC.stopCinematic		= &UI_StopCinematic;
	uiInfo.uiDC.textHeight			= &Text_Height;
	uiInfo.uiDC.textWidth			= &Text_Width;
	uiInfo.uiDC.feederItemImage		= &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText		= &UI_FeederItemText;
#ifdef _IMMERSION
	uiInfo.uiDC.registerForce		= &trap_FF_Register;
	uiInfo.uiDC.startForce			= &trap_FF_Start;
#endif // _IMMERSION
	uiInfo.uiDC.ownerDrawHandleKey	= &UI_OwnerDrawHandleKey;

	UI_Load();

//	String_Init();

	uiInfo.uiDC.whiteShader = ui.R_RegisterShaderNoMip( "white" );

	AssetCache();

	start = Sys_Milliseconds();

	uis.debugMode = qfalse;
	
	Menus_CloseAll();

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = (int)trap_Cvar_VariableValue("color")-1;
	if (uiInfo.effectsColor < 0)
	{
		uiInfo.effectsColor = 0;
	}
	uiInfo.effectsColor = gamecodetoui[uiInfo.effectsColor];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	Cvar_Set("cg_endcredits", "0");	// Reset value
	Cvar_Set("ui_hidelang",	"0");	//default to western, taiwanese config will set to hide european and show taiwanese on menu
}


/*
=================
UI_RegisterCvars
=================
*/
static void UI_RegisterCvars( void ) 
{
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) 
	{
		Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}


/*
=================
UI_ParseMenu
=================
*/
void UI_ParseMenu(const char *menuFile) 
{
	char	*buffer,*holdBuffer,*token2;
	int len;
//	pc_token_t token;

	Com_DPrintf("Parsing menu file:%s\n", menuFile);

	len = PC_StartParseSession(menuFile,&buffer);

	holdBuffer = buffer;

	if (len<=0) 
	{
		Com_Printf("UI_ParseMenu: Unable to load menu %s\n", menuFile);
		return;
	}

	while ( 1 ) 
	{

		token2 = PC_ParseExt();

		if (!*token2)
		{
			break;
		}
/*
		if ( menuCount == MAX_MENUS ) 
		{
			PC_ParseWarning("Too many menus!");		
			break;
		}
*/
		if ( *token2 == '{') 
		{
			continue;
		}
		else if ( *token2 == '}' ) 
		{
			break;
		}
		else if (Q_stricmp(token2, "assetGlobalDef") == 0) 
		{
			if (Asset_Parse(&holdBuffer)) 
			{
				continue;
			} 
			else 
			{
				break;
			}
		}
		else if (Q_stricmp(token2, "menudef") == 0) 
		{
			// start a new menu
			Menu_New(holdBuffer);
			continue;
		}

		PC_ParseWarning(va("Invalid keyword '%s'",token2));		
	}

	PC_EndParseSession(buffer);
	
}

/*
=================
Load_Menu
	Load current menu file
=================
*/
qboolean Load_Menu(const char **holdBuffer) 
{
	const char	*token2;

	token2 = COM_ParseExt( holdBuffer, qtrue );

	if (!token2)
	{
		return qfalse;
	}

	if (*token2 != '{') 
	{
		return qfalse;
	}

	while ( 1 ) 
	{
		token2 = COM_ParseExt( holdBuffer, qtrue );

		if ((!token2) || (token2 == 0))
		{
			return qfalse;
		}
			
		if ( *token2 == '}' ) 
		{
			return qtrue;
		}

//#ifdef _DEBUG
//		extern void UI_Debug_AddMenuFilePath(const char *);
//		UI_Debug_AddMenuFilePath(token2);
//#endif
		UI_ParseMenu(token2); 

	}
	return qfalse;
}

/*
=================
UI_LoadMenus
	Load all menus based on the files listed in the data file in menuFile (default "ui/menus.txt")
=================
*/
void UI_LoadMenus(const char *menuFile, qboolean reset) 
{
//	pc_token_t token;
//	int handle;
	int start;

	char *buffer;
	const char *holdBuffer;
	int len;

	start = Sys_Milliseconds();

	len = ui.FS_ReadFile(menuFile,(void **) &buffer);

	if (len<1) 
	{
		Com_Printf( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		len = ui.FS_ReadFile("ui/menus.txt",(void **) &buffer);

		if (len<1) 
		{
			Com_Error( ERR_FATAL, "%s", va("default menu file not found: ui/menus.txt, unable to continue!\n", menuFile ));
			return;
		}
	}

	if (reset) 
	{
		Menu_Reset();
	}

	const char	*token2;
	holdBuffer = buffer;
	while ( 1 ) 
	{
		token2 = COM_ParseExt( &holdBuffer, qtrue );
		if (!*token2)
		{
			break;
		}

		if( *token2 == 0 || *token2 == '}')			// End of the menus file 
		{
			break;
		}

		if (*token2 == '{') 
		{
				continue;
		}
		else if (Q_stricmp(token2, "loadmenu") == 0) 
		{
			if (Load_Menu(&holdBuffer)) 
			{
				continue;
			} 
			else 
			{
				break;
			}
		} 
		else
		{
			Com_Printf("Unknown keyword '%s' in menus file %s\n", token2, menuFile);
		} 
	}

	Com_Printf("UI menu load time = %d milli seconds\n", Sys_Milliseconds() - start);

	ui.FS_FreeFile( buffer );	//let go of the buffer
}

/*
=================
UI_Load
=================
*/
void UI_Load(void) 
{
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();
	char *menuSet = UI_Cvar_VariableString("ui_menuFiles");

	// sod it, parse every menu strip file until we find a gap in the sequence...
	//
	for (int i=0; i<10; i++)
	{
		if (!ui.SP_Register(va("menus%d",i), /*SP_REGISTER_REQUIRED|*/SP_REGISTER_MENU))
			break;
	}


	if (menu && menu->window.name) 
	{
		strcpy(lastName, menu->window.name);
	}

	if (menuSet == NULL || menuSet[0] == '\0') 
	{
		menuSet = "ui/menus.txt";
	}

	String_Init();

//	registeredFontCount = 0;

//	UI_ParseGameInfo("gameinfo.txt");
///*	UI_LoadArenas();

	UI_LoadMenus(menuSet, qtrue);
//	Menus_CloseAll();
}

/*
=================
Asset_Parse
=================
*/
qboolean Asset_Parse(char **buffer) 
{
	char		*token;
	char		*tempStr;
	int			pointSize;

	token = PC_ParseExt();

	if (!token)
	{
		return qfalse;
	}

	if (*token != '{') 
	{
		return qfalse;
	}
    
	while ( 1 ) 
	{

		token = PC_ParseExt();

		if (!token)
		{
			return qfalse;
		}

		if (*token == '}') 
		{
			return qtrue;
		}

		// font
		if (Q_stricmp(token, "mediumFont") == 0) 
		{
			if (!PC_ParseStringMem(&uiInfo.uiDC.Assets.fontStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'font'");
				return qfalse;
			}

			if (PC_ParseInt(&pointSize))
			{
				PC_ParseWarning("Bad 2nd parameter for keyword 'font'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.qhMediumFont = UI_RegisterFont(uiInfo.uiDC.Assets.fontStr);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token, "smallFont") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'smallFont'");
				return qfalse;
			}

			if (PC_ParseInt(&pointSize))
			{
				PC_ParseWarning("Bad 2nd parameter for keyword 'smallFont'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.qhSmallFont = UI_RegisterFont(tempStr);
			continue;
		}

		if (Q_stricmp(token, "bigFont") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'bigFont'");
				return qfalse;
			}

			if (PC_ParseInt(&pointSize))
			{
				PC_ParseWarning("Bad 2nd parameter for keyword 'bigFont'");
				return qfalse;
			}

			// findmste
			uiInfo.uiDC.Assets.qhBigFont = UI_RegisterFont(tempStr);//, pointSize, &uiInfo.uiDC.Assets.bigFont);
			continue;
		}


		if (Q_stricmp(token, "stripedFile") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'stripedFile'");
				return qfalse;
			}

			char sTemp[1024];
			Q_strncpyz( sTemp, tempStr,  sizeof(sTemp) );
			if (!ui.SP_Register(sTemp, /*SP_REGISTER_REQUIRED|*/SP_REGISTER_MENU))
			{
				PC_ParseWarning(va("(.SP file \"%s\" not found)",sTemp));
				//return qfalse;	// hmmm... dunno about this, don't want to break scripts for just missing subtitles
			}
			else
			{
//				extern void AddMenuPackageRetryKey(const char *);
//				AddMenuPackageRetryKey(sTemp);
			}

			continue;
		}

		// gradientbar
		if (Q_stricmp(token, "gradientbar") == 0) 
		{
			if (!PC_ParseStringMem((const char **)&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'gradientbar'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = ui.R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token, "menuEnterSound") == 0) 
		{
			if (!PC_ParseStringMem((const char **)&tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuEnterSound'");
				return qfalse;
			}

			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token, "menuExitSound") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuExitSound'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token, "itemFocusSound") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'itemFocusSound'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token, "menuBuzzSound") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuBuzzSound'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

#ifdef _IMMERSION

		if (Q_stricmp(token, "menuEnterForce") == 0)
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuEnterForce'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterForce = trap_FF_Register( tempStr );
			continue;
		}

		if (Q_stricmp(token, "menuExitForce") == 0)
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuExitForce'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitForce = trap_FF_Register( tempStr );
			continue;
		}

		if (Q_stricmp(token, "itemFocusForce") == 0)
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'itemFocusForce'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusForce = trap_FF_Register( tempStr );
			continue;
		}

		if (Q_stricmp(token, "menuBuzzForce") == 0)
		{
			if (!PC_ParseStringMem((const char **) &tempStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'menuBuzzForce'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzForce = trap_FF_Register( tempStr );
			continue;
		}

#endif // _IMMERSION
		if (Q_stricmp(token, "cursor") == 0) 
		{
			if (!PC_ParseStringMem((const char **) &uiInfo.uiDC.Assets.cursorStr))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'cursor'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = ui.R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "fadeClamp") == 0) 
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.fadeClamp))
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'fadeClamp'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "fadeCycle") == 0) 
		{
			if (PC_ParseInt(&uiInfo.uiDC.Assets.fadeCycle)) 
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'fadeCycle'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "fadeAmount") == 0) 
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.fadeAmount)) 
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'fadeAmount'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "shadowX") == 0) 
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.shadowX)) 
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'shadowX'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "shadowY") == 0) 
		{
			if (PC_ParseFloat(&uiInfo.uiDC.Assets.shadowY)) 
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'shadowY'");
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token, "shadowColor") == 0) 
		{
			if (PC_ParseColor(&uiInfo.uiDC.Assets.shadowColor)) 
			{
				PC_ParseWarning("Bad 1st parameter for keyword 'shadowColor'");
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

	}


	PC_ParseWarning(va("Invalid keyword '%s'",token));
	return qfalse;
}

/*
=================
UI_Update
=================
*/
static void UI_Update(const char *name) 
{
	int	val = trap_Cvar_VariableValue(name);


	if (Q_stricmp(name, "s_khz") == 0) 
	{
		ui.Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		return;
	}
#ifdef _IMMERSION
	if (Q_stricmp(name, "ff") == 0) 
	{
		ui.Cmd_ExecuteText( EXEC_APPEND, "ff_restart\n");
		return;
	}
#endif // _IMMERSION

	if (Q_stricmp(name, "ui_SetName") == 0) 
	{
		Cvar_Set( "name", UI_Cvar_VariableString("ui_Name"));
 	} 
	else if (Q_stricmp(name, "ui_setRate") == 0) 
	{
		float rate = trap_Cvar_VariableValue("rate");
		if (rate >= 5000) 
		{
			Cvar_Set("cl_maxpackets", "30");
			Cvar_Set("cl_packetdup", "1");
		} 
		else if (rate >= 4000) 
		{
			Cvar_Set("cl_maxpackets", "15");
			Cvar_Set("cl_packetdup", "2");		// favor less prediction errors when there's packet loss
		} 
		else 
		{
			Cvar_Set("cl_maxpackets", "15");
			Cvar_Set("cl_packetdup", "1");		// favor lower bandwidth
		}
	} 
	else if (Q_stricmp(name, "ui_GetName") == 0) 
	{
		Cvar_Set( "ui_Name", UI_Cvar_VariableString("name"));
 	} 
	else if (Q_stricmp(name, "ui_r_colorbits") == 0) 
	{
		switch (val) 
		{
			case 0:
				Cvar_SetValue( "ui_r_depthbits", 0 );
				break;

			case 16:
				Cvar_SetValue( "ui_r_depthbits", 16 );
				break;

			case 32:
				Cvar_SetValue( "ui_r_depthbits", 24 );
				break;
		}
	} 
	else if (Q_stricmp(name, "ui_r_lodbias") == 0) 
	{
		switch (val) 
		{
			case 0:
				Cvar_SetValue( "ui_r_subdivisions", 4 );
				break;
			case 1:
				Cvar_SetValue( "ui_r_subdivisions", 12 );
				break;

			case 2:
				Cvar_SetValue( "ui_r_subdivisions", 20 );
				break;
		}
	} 
	else if (Q_stricmp(name, "ui_r_glCustom") == 0) 
	{
		switch (val) 
		{
			case 0:	// high quality

				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 4 );
				Cvar_SetValue( "ui_r_lodbias", 0 );
				Cvar_SetValue( "ui_r_colorbits", 32 );
				Cvar_SetValue( "ui_r_depthbits", 24 );
				Cvar_SetValue( "ui_r_picmip", 0 );
				Cvar_SetValue( "ui_r_mode", 4 );
				Cvar_SetValue( "ui_r_texturebits", 32 );
				Cvar_SetValue( "ui_r_fastSky", 0 );
				Cvar_SetValue( "ui_r_inGameVideo", 1 );
				//Cvar_SetValue( "ui_cg_shadows", 2 );//stencil
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 1: // normal 
				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 4 );
				Cvar_SetValue( "ui_r_lodbias", 0 );
				Cvar_SetValue( "ui_r_colorbits", 0 );
				Cvar_SetValue( "ui_r_depthbits", 24 );
				Cvar_SetValue( "ui_r_picmip", 1 );
				Cvar_SetValue( "ui_r_mode", 3 );
				Cvar_SetValue( "ui_r_texturebits", 0 );
				Cvar_SetValue( "ui_r_fastSky", 0 );
				Cvar_SetValue( "ui_r_inGameVideo", 1 );
				//Cvar_SetValue( "ui_cg_shadows", 2 );
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 2: // fast

				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 12 );
				Cvar_SetValue( "ui_r_lodbias", 1 );
				Cvar_SetValue( "ui_r_colorbits", 0 );
				Cvar_SetValue( "ui_r_depthbits", 0 );
				Cvar_SetValue( "ui_r_picmip", 2 );
				Cvar_SetValue( "ui_r_mode", 3 );
				Cvar_SetValue( "ui_r_texturebits", 0 );
				Cvar_SetValue( "ui_r_fastSky", 1 );
				Cvar_SetValue( "ui_r_inGameVideo", 0 );
				//Cvar_SetValue( "ui_cg_shadows", 1 );
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;

			case 3: // fastest

				Cvar_SetValue( "ui_r_fullScreen", 1 );
				Cvar_SetValue( "ui_r_subdivisions", 20 );
				Cvar_SetValue( "ui_r_lodbias", 2 );
				Cvar_SetValue( "ui_r_colorbits", 16 );
				Cvar_SetValue( "ui_r_depthbits", 16 );
				Cvar_SetValue( "ui_r_mode", 3 );
				Cvar_SetValue( "ui_r_picmip", 3 );
				Cvar_SetValue( "ui_r_texturebits", 16 );
				Cvar_SetValue( "ui_r_fastSky", 1 );
				Cvar_SetValue( "ui_r_inGameVideo", 0 );
				//Cvar_SetValue( "ui_cg_shadows", 0 );
				Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
		}
	} 
	else if (Q_stricmp(name, "ui_mousePitch") == 0) 
	{
		if (val == 0) 
		{
			Cvar_SetValue( "m_pitch", 0.022f );
		} 
		else 
		{
			Cvar_SetValue( "m_pitch", -0.022f );
		}
	}
}

#define ASSET_SCROLLBAR             "gfx/menus/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "gfx/menus/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "gfx/menus/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "gfx/menus/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "gfx/menus/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "gfx/menus/scrollbar_thumb.tga"


/*
=================
AssetCache
=================
*/
void AssetCache(void) 
{
//	int n;
	uiInfo.uiDC.Assets.scrollBar = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = ui.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = ui.R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );

	uiInfo.uiDC.Assets.sliderBar = ui.R_RegisterShaderNoMip( "menu/new/slider" );
	uiInfo.uiDC.Assets.sliderThumb = ui.R_RegisterShaderNoMip( "menu/new/sliderthumb");
/*
	for( n = 0; n < NUM_CROSSHAIRS; n++ ) 
	{
		uiInfo.uiDC.Assets.crosshairShader[n] = ui.R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}
	*/
}

/*
================
_UI_DrawSides
=================
*/
void _UI_DrawSides(float x, float y, float w, float h, float size) 
{
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

/*
================
_UI_DrawTopBottom
=================
*/
void _UI_DrawTopBottom(float x, float y, float w, float h, float size) 
{
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color ) 
{
	trap_R_SetColor( color );

	_UI_DrawTopBottom(x, y, width, height, size);
	_UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) 
{
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) 
	{
		Cvar_Update( cv->vmCvar );
	}
}

/*
=================
UI_DrawEffects
=================
*/
static void UI_DrawEffects(rectDef_t *rect, float scale, vec4_t color) 
{
	UI_DrawHandlePic( rect->x, rect->y - 14, 128, 8, uiInfo.uiDC.Assets.fxBasePic );
	UI_DrawHandlePic( rect->x + uiInfo.effectsColor * 16 + 8, rect->y - 16, 16, 12, uiInfo.uiDC.Assets.fxPic[uiInfo.effectsColor] );
}

/*
=================
UI_Version
=================
*/
static void UI_Version(rectDef_t *rect, float scale, vec4_t color, int iFontIndex) 
{
	int width;
	
	width = DC->textWidth(Q3_VERSION, scale, 0);

	DC->drawText(rect->x - width, rect->y, scale, color, Q3_VERSION, 0, ITEM_TEXTSTYLE_SHADOWED, iFontIndex);
}

/*
=================
UI_DrawKeyBindStatus
=================
*/
static void UI_DrawKeyBindStatus(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iFontIndex) 
{
	if (Display_KeyBindPending()) 
	{
		Text_Paint(rect->x, rect->y, scale, color, ui.SP_GetStringTextString("MENUS_WAITINGFORKEY"), 0, textStyle, iFontIndex);
	} 
	else 
	{
//		Text_Paint(rect->x, rect->y, scale, color, ui.SP_GetStringTextString("MENUS_ENTERTOCHANGE"), 0, textStyle, iFontIndex);
	}
}

/*
=================
UI_DrawKeyBindStatus
=================
*/
static void UI_DrawGLInfo(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iFontIndex) 
{
#define MAX_LINES 64
	char buff[4096];
	char * eptr = buff;
	const char *lines[MAX_LINES];
	int y, numLines=0, i=0;

	y = rect->y;	
	Text_Paint(rect->x, y, scale, color, va("GL_VENDOR: %s",uiInfo.uiDC.glconfig.vendor_string), rect->w, textStyle, iFontIndex);
	y += 15;
	Text_Paint(rect->x, y, scale, color, va("GL_VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string), rect->w, textStyle, iFontIndex);
	y += 15;
	Text_Paint(rect->x, y, scale, color, "GL_PIXELFORMAT:", rect->w, textStyle, iFontIndex);
	y += 15;
	Text_Paint(rect->x, y, scale, color, va ("Color(%d-bits) Z(%d-bits) stencil(%d-bits)",uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits), rect->w, textStyle, iFontIndex);
	y += 15;
	// build null terminated extension strings
	Q_strncpyz(buff, uiInfo.uiDC.glconfig.extensions_string, sizeof(buff));
	int testy=y-16;
	while ( testy <= rect->y + rect->h && *eptr && (numLines < MAX_LINES) )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ') 
		{
			lines[numLines++] = eptr;
			testy+=16;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	numLines--;
	while (i < numLines) 
	{
		Text_Paint(rect->x, y, scale, color, lines[i++], rect->w, textStyle, iFontIndex);
		y += 16;
	}
}


/*
===================
UI_DataPad_DrawIconBackground
===================
*/
void UI_DataPad_DrawIconBackground(void)
{
//	ui.R_SetColor( colorTable[CT_WHITE] );

//	CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, cgs.media.weaponProngsOff);
//	CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, cgs.media.weaponProngsOff);

}

/*
=================
UI_DataPad_Inventory
=================
*/
/*
static void UI_DataPad_Inventory(rectDef_t *rect, float scale, vec4_t color, int iFontIndex) 
{
	Text_Paint(rect->x, rect->y, scale, color, "INVENTORY", 0, 1, iFontIndex);
}
*/
/*
=================
UI_DataPad_ForcePowers
=================
*/
/*
static void UI_DataPad_ForcePowers(rectDef_t *rect, float scale, vec4_t color, int iFontIndex) 
{
	Text_Paint(rect->x, rect->y, scale, color, "FORCE POWERS", 0, 1, iFontIndex);
}
*/

static void UI_DrawCrosshair(rectDef_t *rect, float scale, vec4_t color) {
 	trap_R_SetColor( color );
	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
 	trap_R_SetColor( NULL );
}


/*
=================
UI_OwnerDraw
=================
*/
static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int iFontIndex) 
{
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch (ownerDraw) 
	{
		case UI_EFFECTS:
			UI_DrawEffects(&rect, scale, color);
			break;
		case UI_VERSION:
			UI_Version(&rect, scale, color, iFontIndex);
			break;

		case UI_DATAPAD_MISSION:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_OBJECTIVES);
			break;

		case UI_DATAPAD_WEAPONS:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_WEAPONS);
			break;

		case UI_DATAPAD_INVENTORY:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_INVENTORY);
			break;

		case UI_DATAPAD_FORCEPOWERS:
			ui.Draw_DataPad(DP_HUD);
			ui.Draw_DataPad(DP_FORCEPOWERS);
			break;

		case UI_ALLMAPS_SELECTION://saved game thumbnail
			ui.DrawStretchRaw( x, y+h, w, -h, SG_SCR_WIDTH, SG_SCR_HEIGHT, screenShotBuf, 0, qtrue );
			ui.R_Font_DrawString(	x,		// int ox
									y+h,	// int oy
									s_savedata[s_savegame.currentLine].currentSaveFileMap,	// const char *text
									color,	// paletteRGBA_c c
									iFontIndex,	// const int iFontHandle
									w,//-1,		// iMaxPixelWidth (-1 = none)
									scale	// const float scale = 1.0f
									);
			break;
		case UI_PREVIEWCINEMATIC:
			// FIXME BOB - make this work?
//			UI_DrawPreviewCinematic(&rect, scale, color);
			break;
		case UI_CROSSHAIR:
			UI_DrawCrosshair(&rect, scale, color);
			break;
		case UI_GLINFO:
			UI_DrawGLInfo(&rect,scale, color, textStyle, iFontIndex);
			break;
		case UI_KEYBINDSTATUS:
			UI_DrawKeyBindStatus(&rect,scale, color, textStyle, iFontIndex);
			break;
		default:
		  break;
	}

}

/*
=================
UI_UpdateCvars
=================
*/
static qboolean UI_OwnerDrawVisible(int flags) 
{
	qboolean vis = qtrue;

	while (flags) 
	{
		// FIXME BOB
/*		if (flags & UI_SHOW_FFA) 
		{
			if (trap_Cvar_VariableValue("g_gametype") != GT_FFA) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}
*/

		// FIXME BOB
/*		if (flags & UI_SHOW_NOTFFA) 
		{
			if (trap_Cvar_VariableValue("g_gametype") == GT_FFA) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}

		if (flags & UI_SHOW_LEADER) 
		{
			// these need to show when this client can give orders to a player or a group
			if (!uiInfo.teamLeader) 
			{
				vis = qfalse;
			} 
			else 
			{
				// if showing yourself
				if (ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber) 
				{	
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_LEADER;
		} 

		if (flags & UI_SHOW_NOTLEADER) 
		{
			// these need to show when this client is assigning their own status or they are NOT the leader
			if (uiInfo.teamLeader) {
				// if not showing yourself
				if (!(ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber)) { 
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_NOTLEADER;
		} 

		if (flags & UI_SHOW_FAVORITESERVERS) 
		{
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer != AS_FAVORITES) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		} 

		if (flags & UI_SHOW_NOTFAVORITESERVERS) 
		{
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer == AS_FAVORITES) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		} 

		if (flags & UI_SHOW_ANYTEAMGAME) 
		{
			if (uiInfo.gameTypes[ui_gameType.integer].gtEnum <= GT_TEAM ) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYTEAMGAME;
		} 

		if (flags & UI_SHOW_ANYNONTEAMGAME) 
		{
			if (uiInfo.gameTypes[ui_gameType.integer].gtEnum > GT_TEAM ) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		} 

		if (flags & UI_SHOW_NETANYTEAMGAME) 
		{
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum <= GT_TEAM ) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		} 

		if (flags & UI_SHOW_NETANYNONTEAMGAME) 
		{
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum > GT_TEAM ) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		} 

		if (flags & UI_SHOW_NEWHIGHSCORE) 
		{
			if (uiInfo.newHighScoreTime < uiInfo.uiDC.realTime) 
			{
				vis = qfalse;
			} else {
				if (uiInfo.soundHighScore) 
				{
					if (trap_Cvar_VariableValue("sv_killserver") == 0) 
					{
						// wait on server to go down before playing sound
						trap_S_StartLocalSound(uiInfo.newHighScoreSound, CHAN_ANNOUNCER);
						uiInfo.soundHighScore = qfalse;
					}
				}
			}
			flags &= ~UI_SHOW_NEWHIGHSCORE;
		} 

		if (flags & UI_SHOW_NEWBESTTIME) 
		{
			if (uiInfo.newBestTime < uiInfo.uiDC.realTime) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NEWBESTTIME;
		} 
*/

		if (flags & UI_SHOW_DEMOAVAILABLE) 
		{
			if (!uiInfo.demoAvailable) 
			{
				vis = qfalse;
			}
			flags &= ~UI_SHOW_DEMOAVAILABLE;
		} 
		else 
		{
			flags = 0;
		}
	}
	return vis;
}

/*
=================
Text_Width
=================
*/
int Text_Width(const char *text, float scale, int iFontIndex) 
{
	// temp code until Bob retro-fits all menus to have font specifiers...
	//
	if ( iFontIndex == 0 )
	{
		iFontIndex = uiInfo.uiDC.Assets.qhMediumFont;
	}
	return ui.R_Font_StrLenPixels(text, iFontIndex, scale);
}

/*
=================
UI_OwnerDrawWidth
=================
*/
int UI_OwnerDrawWidth(int ownerDraw, float scale) 
{
//	int i, h, value;
//	const char *text;
	const char *s = NULL;


	switch (ownerDraw) 
	{
	case UI_KEYBINDSTATUS:
		if (Display_KeyBindPending()) 
		{
			s = ui.SP_GetStringTextString("MENUS_WAITINGFORKEY");
		} 
		else 
		{
//			s = ui.SP_GetStringTextString("MENUS_ENTERTOCHANGE");
		}
		break;
	
	// FIXME BOB
//	case UI_SERVERREFRESHDATE:
//		s = UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer));
//		break;
    default:
      break;
	}

	if (s) 
	{
		return Text_Width(s, scale, 0);
	}
	return 0;
}

/*
=================
Text_Height
=================
*/
int Text_Height(const char *text, float scale, int iFontIndex) 
{
	// temp until Bob retro-fits all menu files with font specifiers...
	//
	if ( iFontIndex == 0 )
	{
		iFontIndex = uiInfo.uiDC.Assets.qhMediumFont;
	}
	return ui.R_Font_HeightPixels(iFontIndex, scale);
}


/*
=================
UI_MouseEvent
=================
*/
void _UI_MouseEvent( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if (uiInfo.uiDC.cursorx < 0)
	{
		uiInfo.uiDC.cursorx = 0;
	}
	else if (uiInfo.uiDC.cursorx > SCREEN_WIDTH)
	{
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;
	if (uiInfo.uiDC.cursory < 0)
	{
		uiInfo.uiDC.cursory = 0;
	}
	else if (uiInfo.uiDC.cursory > SCREEN_HEIGHT)
	{
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	if (Menu_Count() > 0) 
	{
    //menuDef_t *menu = Menu_GetFocused();
    //Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
	}

}

/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down ) 
{
/*	extern qboolean SwallowBadNumLockedKPKey( int iKey );
	if (SwallowBadNumLockedKPKey(key)){
		return;
	}
*/

	if (Menu_Count() > 0) 
	{
		menuDef_t *menu = Menu_GetFocused();
		if (menu) 
		{
			if (key == A_ESCAPE && down && !Menus_AnyFullScreenVisible()) 
			{
				Menus_CloseAll();
			} 
			else 
			{
				Menu_HandleKey(menu, key, down );
			}
		} 
		else 
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
		}
	}
}

/*
=================
UI_Report
=================
*/
void UI_Report(void) 
{
  String_Report();
}
int s_textlanguage_Names[] =
{
//	MNT_ENGLISH,
//	MNT_GERMAN,
//	MNT_FRENCH,
	NULL
};

int s_voicelanguage_Names[] =
{
//	MNT_ENGLISH,
//	MNT_GERMAN,
	NULL
};


int s_subtitle_Names[] =
{
//	MNT_OFF,
//	MNT_ON,
//	MNT_CINEMATIC,
	NULL
};


void Menus_CloseByName(const char *p);

/*
=================
UI_DataPadMenu
=================
*/
void UI_DataPadMenu(void)
{
	int	newForcePower,newObjective;

	Menus_CloseByName("mainhud");

	newForcePower = (int)trap_Cvar_VariableValue("cg_updatedDataPadForcePower1");
	newObjective = (int)trap_Cvar_VariableValue("cg_updatedDataPadObjective");

	if (newForcePower)
	{
		Menus_ActivateByName("datapadForcePowersMenu");
	}
	else if (newObjective)
	{
		Menus_ActivateByName("datapadMissionMenu");
	}
	else
	{
		Menus_ActivateByName("datapadMissionMenu");
	}
	ui.Key_SetCatcher( KEYCATCH_UI );

}

/*
=================
UI_InGameMenu
=================
*/
void UI_InGameMenu(const char*menuID)
{
	ui.PrecacheScreenshot();

	Menus_CloseByName("mainhud");

	if (menuID)
	{
		Menus_ActivateByName(menuID);
	}
	else
	{
		Menus_ActivateByName("ingameMainMenu");
	}
	ui.Key_SetCatcher( KEYCATCH_UI );
}

qboolean _UI_IsFullscreen( void ) 
{
	return Menus_AnyFullScreenVisible();
}

/*
=======================================================================

MAIN MENU

=======================================================================
*/

static const char *s_keyboardlanguage_Names[] =
{
	"AMERICAN",
	"DEUTSCH",
	"FRANCAIS",
	"ESPANOL",
	"ITALIANO",
	0
};

/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu(void)
{
	char buf[256];
	ui.Cvar_Set("sv_killserver", "1");	// let the demo server know it should shut down

	ui.Key_SetCatcher( KEYCATCH_UI );

	Menus_OpenByName("mainMenu");
	ui.Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof(buf));
	if (strlen(buf)) {
		Menus_ActivateByName("error_popmenu");
	}
}


/*
=================
Menu_Cache
=================
*/
void Menu_Cache( void )
{
	uis.cursor		= ui.R_RegisterShaderNoMip( "menu/new/crosshairb");
	// Common menu graphics
	uis.whiteShader = ui.R_RegisterShader( "white" );
	uis.menuBackShader = ui.R_RegisterShader( "menu/art/unknownmap" );
}

/*
=================
UI_UpdateVideoSetup

Copies the temporary user interface version of the video cvars into
their real counterparts.  This is to create a interface which allows 
you to discard your changes if you did something you didnt want
=================
*/
void UI_UpdateVideoSetup ( void )
{
	Cvar_Set ( "r_mode", Cvar_VariableString ( "ui_r_mode" ) );
	Cvar_Set ( "r_fullscreen", Cvar_VariableString ( "ui_r_fullscreen" ) );
	Cvar_Set ( "r_colorbits", Cvar_VariableString ( "ui_r_colorbits" ) );
	Cvar_Set ( "r_lodbias", Cvar_VariableString ( "ui_r_lodbias" ) );
	Cvar_Set ( "r_picmip", Cvar_VariableString ( "ui_r_picmip" ) );
	Cvar_Set ( "r_texturebits", Cvar_VariableString ( "ui_r_texturebits" ) );
	Cvar_Set ( "r_texturemode", Cvar_VariableString ( "ui_r_texturemode" ) );
	Cvar_Set ( "r_detailtextures", Cvar_VariableString ( "ui_r_detailtextures" ) );
	Cvar_Set ( "r_ext_compress_textures", Cvar_VariableString ( "ui_r_ext_compress_textures" ) );
	Cvar_Set ( "r_depthbits", Cvar_VariableString ( "ui_r_depthbits" ) );
	Cvar_Set ( "r_subdivisions", Cvar_VariableString ( "ui_r_subdivisions" ) );
	Cvar_Set ( "r_fastSky", Cvar_VariableString ( "ui_r_fastSky" ) );
	Cvar_Set ( "r_inGameVideo", Cvar_VariableString ( "ui_r_inGameVideo" ) );
	Cvar_Set ( "r_allowExtensions", Cvar_VariableString ( "ui_r_allowExtensions" ) );
	Cvar_Set ( "cg_shadows", Cvar_VariableString ( "ui_cg_shadows" ) );
	Cvar_Set ( "ui_r_modified", "0" );

	Cbuf_ExecuteText( EXEC_APPEND, "vid_restart;" );
}

/*
=================
UI_GetVideoSetup

Retrieves the current actual video settings into the temporary user
interface versions of the cvars.
=================
*/
void UI_GetVideoSetup ( void )
{
	// Make sure the cvars are registered as read only.
	Cvar_Register ( NULL, "ui_r_glCustom",				"4", CVAR_ROM|CVAR_ARCHIVE );

	Cvar_Register ( NULL, "ui_r_mode",					"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_fullscreen",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_colorbits",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_lodbias",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_picmip",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_texturebits",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_texturemode",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_detailtextures",		"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_ext_compress_textures",	"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_depthbits",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_subdivisions",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_fastSky",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_inGameVideo",			"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_allowExtensions",		"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_cg_shadows",				"0", CVAR_ROM );
	Cvar_Register ( NULL, "ui_r_modified",				"0", CVAR_ROM );
	
	// Copy over the real video cvars into their temporary counterparts
	Cvar_Set ( "ui_r_mode", Cvar_VariableString ( "r_mode" ) );
	Cvar_Set ( "ui_r_colorbits", Cvar_VariableString ( "r_colorbits" ) );
	Cvar_Set ( "ui_r_fullscreen", Cvar_VariableString ( "r_fullscreen" ) );
	Cvar_Set ( "ui_r_lodbias", Cvar_VariableString ( "r_lodbias" ) );
	Cvar_Set ( "ui_r_picmip", Cvar_VariableString ( "r_picmip" ) );
	Cvar_Set ( "ui_r_texturebits", Cvar_VariableString ( "r_texturebits" ) );
	Cvar_Set ( "ui_r_texturemode", Cvar_VariableString ( "r_texturemode" ) );
	Cvar_Set ( "ui_r_detailtextures", Cvar_VariableString ( "r_detailtextures" ) );
	Cvar_Set ( "ui_r_ext_compress_textures", Cvar_VariableString ( "r_ext_compress_textures" ) );
	Cvar_Set ( "ui_r_depthbits", Cvar_VariableString ( "r_depthbits" ) );
	Cvar_Set ( "ui_r_subdivisions", Cvar_VariableString ( "r_subdivisions" ) );
	Cvar_Set ( "ui_r_fastSky", Cvar_VariableString ( "r_fastSky" ) );
	Cvar_Set ( "ui_r_inGameVideo", Cvar_VariableString ( "r_inGameVideo" ) );
	Cvar_Set ( "ui_r_allowExtensions", Cvar_VariableString ( "r_allowExtensions" ) );
	Cvar_Set ( "ui_cg_shadows", Cvar_VariableString ( "cg_shadows" ) );
	Cvar_Set ( "ui_r_modified", "0" );
}

char GoToMenu[1024];

/*
=================
Menus_SaveGoToMenu
=================
*/
void Menus_SaveGoToMenu(const char *menuTo)
{
	memcpy(GoToMenu, menuTo, sizeof(GoToMenu));
}

/*
=================
UI_CheckVid1Data
=================
*/
void UI_CheckVid1Data(const char *menuTo,const char *warningMenuName)
{
	menuDef_t *menu;
	itemDef_t *applyChanges;

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: No videoMenu was found. Video data could not be checked\n");
		return;
	}

	applyChanges = (itemDef_s *) Menu_FindItemByName(menu, "applyChanges");

	if (!applyChanges)
	{
//		Menus_CloseAll();
		Menus_OpenByName(menuTo);
		return;
	}

	if ((applyChanges->window.flags & WINDOW_VISIBLE))	// Is the APPLY CHANGES button active?
	{
//		Menus_SaveGoToMenu(menuTo);							// Save menu you're going to
//		Menus_HideItems(menu->window.name);					// HIDE videMenu in case you have to come back
		Menus_OpenByName(warningMenuName);				// Give warning
	}
	else
	{
//		Menus_CloseAll();
//		Menus_OpenByName(menuTo);
	}
}

/*
=================
UI_ResetDefaults
=================
*/
void UI_ResetDefaults( void )
{
	ui.Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	ui.Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	ui.Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
	Cvar_Set("com_introPlayed", "1" );
}

/*
=======================
UI_SortSaveGames
=======================
*/
static int UI_SortSaveGames( const void *A, const void *B ) 
{

	const int &a = ((savedata_t*)A)->currentSaveFileDateTime;
	const int &b = ((savedata_t*)B)->currentSaveFileDateTime;

	if (a > b)
	{
		return -1;
	}
	else
	{
		return (a < b);
	}
}

/*
=================
ReadSaveDirectory
=================
*/
void ReadSaveDirectory (void)
{
	int		i;
	char	*holdChar;
	int		len;
	int		fileCnt;
	// Clear out save data
	memset(s_savedata,0,sizeof(s_savedata));
	s_savegame.saveFileCnt = 0;
	Cvar_Set("ui_gameDesc", "" );	// Blank out comment 
	Cvar_Set("ui_SelectionOK", "0" );
	memset( screenShotBuf,0,(SG_SCR_WIDTH * SG_SCR_HEIGHT * 4)); //blank out sshot

	// Get everything in saves directory
	fileCnt = ui.FS_GetFileList("saves", ".sav", s_savegame.listBuf, LISTBUFSIZE );

	Cvar_Set("ui_ResumeOK", "0" );
	holdChar = s_savegame.listBuf;
	for ( i = 0; i < fileCnt; i++ ) 
	{
		// strip extension
		len = strlen( holdChar );
		holdChar[len-4] = '\0';

		if	( Q_stricmp("current",holdChar)!=0 )
		{
			time_t result;
			if (Q_stricmp("auto",holdChar)==0)
			{
				Cvar_Set("ui_ResumeOK", "1" );
			}
			else
			{	// Is this a valid file??? & Get comment of file
				result = ui.SG_GetSaveGameComment(holdChar, s_savedata[s_savegame.saveFileCnt].currentSaveFileComments, s_savedata[s_savegame.saveFileCnt].currentSaveFileMap);
				if (result != 0) // ignore Bad save game 
				{
					s_savedata[s_savegame.saveFileCnt].currentSaveFileName = holdChar;
					s_savedata[s_savegame.saveFileCnt].currentSaveFileDateTime = result;
					
					struct tm *localTime;
					localTime = localtime( &result );
					strcpy(s_savedata[s_savegame.saveFileCnt].currentSaveFileDateTimeString,asctime( localTime ) );
					s_savegame.saveFileCnt++;
					if (s_savegame.saveFileCnt == MAX_SAVELOADFILES)
					{
						break;
					}
				}
			}
		}
		
		holdChar += len + 1;	//move to next item
	}

	qsort( s_savedata, s_savegame.saveFileCnt, sizeof(savedata_t), UI_SortSaveGames );
}
