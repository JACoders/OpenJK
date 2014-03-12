/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "cg_media.h"	//just for cgs....

void CG_TargetCommand_f( void );
extern qboolean	player_locked;
extern void CMD_CGCam_Disable( void );
void CG_NextInventory_f( void );
void CG_PrevInventory_f( void );
void CG_NextForcePower_f( void );
void CG_PrevForcePower_f( void );

/*
=================
CG_TargetCommand_f

=================
*/
void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if ( targetNum == -1 ) {
		return;
	}

	cgi_Argv( 1, test, 4 );
	cgi_SendClientCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}

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
	const char	*targetname;
	static	int	numCams;

	numCams++;
	
	targetname = CG_Argv(1);

	if( !targetname || !targetname[0] )
	{
		targetname = "nameme!";
	}

	CG_Printf( "Camera #%d ('%s') written to: ", numCams, targetname );
	Com_sprintf( text, sizeof(text), "//entity %d\n{\n\"classname\"	\"ref_tag\"\n\"targetname\"	\"%s\"\n\"origin\" \"%i %i %i\"\n\"angles\" \"%i %i %i\"\n\"fov\" \"%i\"\n}\n", numCams, targetname, (int)cg.refdef.vieworg[0], (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], (int)cg.refdefViewAngles[0], (int)cg.refdefViewAngles[1], (int)cg.refdefViewAngles[2], cg_fov.integer );
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
		if ( (cg.snap->ps.saberActive && cg.snap->ps.saberInFlight) || cg.snap->ps.stats[STAT_HEALTH] <= 0)
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
		if ( (cg.snap->ps.saberActive && cg.snap->ps.saberInFlight) || cg.snap->ps.stats[STAT_HEALTH] <= 0 )
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

void CG_LoadHud_f( void) 
{
	const char *hudSet = cg_hudFiles.string;
	if ( hudSet[0] == '\0' ) 
		hudSet = "ui/jk2hud.txt";

	CG_LoadMenus(hudSet);
}

typedef struct {
	const char	*cmd;
	void		(*func)(void);
} consoleCommand_t;

int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((consoleCommand_t*)b)->cmd );
}

/* This array MUST be sorted correctly by alphabetical name field */
static consoleCommand_t	commands[] = {
	{ "cam_disable", CMD_CGCam_Disable },	//gets out of camera mode for debuggin
	{ "cam_enable", CGCam_Enable },	//gets into camera mode for precise camera placement
	{ "dpforcenext", CG_DPNextForcePower_f },
	{ "dpforceprev", CG_DPPrevForcePower_f },
	{ "dpinvnext", CG_DPNextInventory_f },
	{ "dpinvprev", CG_DPPrevInventory_f },
	{ "dpweapnext", CG_DPNextWeapon_f },
	{ "dpweapprev", CG_DPPrevWeapon_f },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "la_zoom", CG_ToggleLAGoggles },
	{ "loadhud", CG_LoadHud_f },
	{ "lock_disable", Lock_Disable },	//player can move now
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "testG2Model", CG_TestG2Model_f},
	{ "testanglespost", CG_TestModelSetAnglespost_f},
	{ "testanglespre", CG_TestModelSetAnglespre_f},
	{ "testanimate", CG_TestModelAnimate_f},
	{ "testgun", CG_TestGun_f },
	{ "testlistbones", CG_ListModelBones_f},
	{ "testlistsurfaces", CG_ListModelSurfaces_f},
	{ "testmodel", CG_TestModel_f },
	{ "testsurface", CG_TestModelSurfaceOnOff_f },
	{ "viewpos", CG_Viewpos_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "writecam", CG_WriteCam_f },
	{ "zoom", CG_ToggleBinoculars },
};

static const size_t numCommands = ARRAY_LEN( commands );

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	consoleCommand_t	*command = NULL;

	command = (consoleCommand_t *)bsearch( CG_Argv( 0 ), commands, numCommands, sizeof( commands[0] ), cmdcmp );

	if ( !command )
		return qfalse;

	command->func();
	return qtrue;
}

static const char *gcmds[] = {
	"entitylist",
	"force_distract",
	"force_grip",
	"force_heal",
	"force_pull",
	"force_speed",
	"force_throw",
	"give",
	"god",
	"invuse",
	"kill",
	"nav",
	"noclip",
	"notarget",
	"npc",
	"playerteam",
	"runscript",
	"saberAttackCycle",
	"saberColor",
	"setForceAll",
	"setobjective",
	"setviewpos",
	"taunt",
	"undying",
	"use_bacta",
	"use_electrobinoculars",
	"use_lightamp_goggles",
	"use_seeker",
	"use_sentry",
	"viewobjective"
};
static const size_t numgcmds = ARRAY_LEN( gcmds );

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	size_t i;

	for ( i = 0 ; i < numCommands ; i++ )
		cgi_AddCommand( commands[i].cmd );

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	for( i = 0; i < numgcmds; i++ )
		cgi_AddCommand( gcmds[i] );
}
