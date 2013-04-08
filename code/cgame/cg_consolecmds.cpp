/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"
#include "cg_media.h"	//just for cgs....

void CG_TargetCommand_f( void );
extern qboolean	player_locked;
extern void CMD_CGCam_Disable( void );
void CG_NextInventory_f( void );
void CG_PrevInventory_f( void );
void CG_NextForcePower_f( void );
void CG_PrevForcePower_f( void );
void CG_LoadHud_f( void );

/*
====================
CG_ColorFromString
====================
*/
/*
static void CG_SetColor_f( void) {

	if (cgi_Argc()==4)
	{
		g_entities[0].client->renderInfo.customRGBA[0] = atoi( CG_Argv(1) );
		g_entities[0].client->renderInfo.customRGBA[1] = atoi( CG_Argv(2) );
		g_entities[0].client->renderInfo.customRGBA[2] = atoi( CG_Argv(3) );
	}
	if (cgi_Argc()==2)
	{
		int val = atoi( CG_Argv(1) );
				
		if ( val < 1 || val > 7 ) {
			g_entities[0].client->renderInfo.customRGBA[0] = 255;
			g_entities[0].client->renderInfo.customRGBA[1] = 255;
			g_entities[0].client->renderInfo.customRGBA[2] = 255;
			return;
		}
		g_entities[0].client->renderInfo.customRGBA[0]=0;
		g_entities[0].client->renderInfo.customRGBA[1]=0;
		g_entities[0].client->renderInfo.customRGBA[2]=0;
		
		if ( val & 1 ) {
			g_entities[0].client->renderInfo.customRGBA[2] = 255;
		}
		if ( val & 2 ) {
			g_entities[0].client->renderInfo.customRGBA[1] = 255;
		}
		if ( val & 4 ) {
			g_entities[0].client->renderInfo.customRGBA[0] = 255;
		}
	}
}
*/
/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("%s (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}

void CG_WriteCam_f (void)
{
	char	text[1024];
	char	*targetname;
	static	int	numCams;

	numCams++;
	
	targetname = (char	*)CG_Argv(1);

	if( !targetname || !targetname[0] )
	{
		targetname = "nameme!";
	}

	CG_Printf( "Camera #%d ('%s') written to: ", numCams, targetname );
	sprintf( text, "//entity %d\n{\n\"classname\"	\"ref_tag\"\n\"targetname\"	\"%s\"\n\"origin\" \"%i %i %i\"\n\"angles\" \"%i %i %i\"\n\"fov\" \"%i\"\n}\n", numCams, targetname, (int)cg.refdef.vieworg[0], (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], (int)cg.refdefViewAngles[0], (int)cg.refdefViewAngles[1], (int)cg.refdefViewAngles[2], cg_fov.integer );
	gi.WriteCam( text );
}

void Lock_Disable ( void )
{
	player_locked = qfalse;
}

extern float cg_zoomFov;	//from cg_view.cpp

void CG_ToggleBinoculars( void )
{
	if ( in_camera || !cg.snap )
	{
		return;
	}

	if ( cg.zoomMode == 0 || cg.zoomMode >= 2 ) // not zoomed or currently zoomed with the disruptor or LA goggles
	{
		if ( (cg.snap->ps.saber[0].Active() && cg.snap->ps.saberInFlight) || cg.snap->ps.stats[STAT_HEALTH] <= 0)
		{//can't select binoculars when throwing saber
			//FIXME: indicate this to the player
			return;
		}

		if ( cg.snap->ps.viewEntity || ( cg_entities[cg.snap->ps.clientNum].currentState.eFlags & ( EF_LOCKED_TO_WEAPON | EF_IN_ATST )))
		{
			// can't zoom when you have a viewEntity or driving an atst or in an emplaced gun
			return;
		}

		cg.zoomMode = 1;
		cg.zoomLocked = qfalse;

		if ( cg.snap->ps.batteryCharge )
		{
			// when you have batteries, you can actually zoom in
			cg_zoomFov = 40.0f;
		}
		else if ( cg.overrides.active & CG_OVERRIDE_FOV )
		{
			cg_zoomFov = cg.overrides.fov;
		}
		else
		{
			cg_zoomFov = cg_fov.value;
		}

		cgi_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomStart );
	}
	else
	{
		cg.zoomMode = 0;
		cg.zoomTime = cg.time;
		cgi_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomEnd );
	}
}

void CG_ToggleLAGoggles( void )
{
	if ( in_camera || !cg.snap)
	{
		return;
	}

	if ( cg.zoomMode == 0 || cg.zoomMode < 3 ) // not zoomed or currently zoomed with the disruptor or regular binoculars
	{
		if ( (cg.snap->ps.saber[0].Active() && cg.snap->ps.saberInFlight) || cg.snap->ps.stats[STAT_HEALTH] <= 0 )
		{//can't select binoculars when throwing saber
			//FIXME: indicate this to the player
			return;
		}

		if ( cg.snap->ps.viewEntity || ( cg_entities[cg.snap->ps.clientNum].currentState.eFlags & ( EF_LOCKED_TO_WEAPON | EF_IN_ATST )))
		{
			// can't zoom when you have a viewEntity or driving an atst or in an emplaced gun
			return;
		}

		cg.zoomMode = 3;
		cg.zoomLocked = qfalse;
		if ( cg.overrides.active & CG_OVERRIDE_FOV )
		{
			cg_zoomFov = cg.overrides.fov;
		}
		else
		{
			cg_zoomFov = cg_fov.value; // does not zoom!!
		}

		cgi_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomStart );
	}
	else
	{
		cg.zoomMode = 0;
		cg.zoomTime = cg.time;
		cgi_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomEnd );
	}
}

static void CG_InfoDown_f( void ) {
//	cg.showInformation = qtrue;
}

static void CG_InfoUp_f( void ) 
{
//	cg.showInformation = qfalse;
}

typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;


static consoleCommand_t	commands[] = {
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
/*
Ghoul2 Insert Start
*/
	{ "testG2Model", CG_TestG2Model_f},
	{ "testsurface", CG_TestModelSurfaceOnOff_f },
	{ "testanglespre", CG_TestModelSetAnglespre_f},
	{ "testanglespost", CG_TestModelSetAnglespost_f},
	{ "testanimate", CG_TestModelAnimate_f},
	{ "testlistbones", CG_ListModelBones_f},
	{ "testlistsurfaces", CG_ListModelSurfaces_f},
/*
Ghoul2 Insert End
*/
	{ "viewpos", CG_Viewpos_f },
	{ "writecam", CG_WriteCam_f },
	{ "+info", CG_InfoDown_f },
	{ "-info", CG_InfoUp_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "cam_disable", CMD_CGCam_Disable },	//gets out of camera mode for debuggin
	{ "cam_enable", CGCam_Enable },	//gets into camera mode for precise camera placement
	{ "lock_disable", Lock_Disable },	//player can move now
	{ "zoom", CG_ToggleBinoculars },
	{ "la_zoom", CG_ToggleLAGoggles },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },
	{ "loadhud", CG_LoadHud_f },
	{ "dpweapnext", CG_DPNextWeapon_f },
	{ "dpweapprev", CG_DPPrevWeapon_f },
	{ "dpinvnext", CG_DPNextInventory_f },
	{ "dpinvprev", CG_DPPrevInventory_f },
	{ "dpforcenext", CG_DPNextForcePower_f },
	{ "dpforceprev", CG_DPPrevForcePower_f },
//	{ "color", CG_SetColor_f },
};


//extern menuDef_t *menuScoreboard;
void Menu_Reset();	

void CG_LoadHud_f( void) 
{
	const char *hudSet;

//	cgi_UI_String_Init();

//	cgi_UI_Menu_Reset();
	
	hudSet = cg_hudFiles.string;
	if (hudSet[0] == '\0') 
	{
		hudSet = "ui/jahud.txt";
	}

	CG_LoadMenus(hudSet);
//	menuScoreboard = NULL;

}

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		cgi_AddCommand( commands[i].cmd );
	}
}
