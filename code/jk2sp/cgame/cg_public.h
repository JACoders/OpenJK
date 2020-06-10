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

#define NUM_EXPLOSION_SHADERS	8
#define NUM_EXPLOSION_FRAMES	3

#define	CMD_BACKUP			64
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	512

#define	SNAPFLAG_RATE_DELAYED		1		// the server withheld a packet to save bandwidth
#define	SNAPFLAG_DROPPED_COMMANDS	2		// the server lost some cmds coming from the client

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct snapshot_s
{
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, SNAPFLAG_DROPPED_COMMANDS

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
									// client side prediction should start with this cmd
	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numConfigstringChanges;	// configstrings that have changed since the last
	int				configstringNum;		// acknowledged snapshot_t (which is usually NOT the previous snapshot!)

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
};

typedef snapshot_s snapshot_t;


/*
==================================================================

functions imported from the main executable

==================================================================
*/

#define	CGAME_IMPORT_API_VERSION	4

typedef enum {
	CG_PRINT,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_MARKFRAGMENTS,
	CG_CM_SNAPPVS,
	CG_S_STARTSOUND,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_REGISTERSHADERNOMIP,
	CG_R_REGISTERFONT,
	CG_R_FONTSTRLENPIXELS,
	CG_R_FONTSTRLENCHARS,
	CG_R_FONTHEIGHTPIXELS,
	CG_R_FONTDRAWSTRING,
	CG_LANGUAGE_ISASIAN,
	CG_LANGUAGE_USESSPACES,
	CG_ANYLANGUAGE_READFROMSTRING,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_GETLIGHTING,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_DRAWSCREENSHOT,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_R_DRAWROTATEPIC,
	CG_R_DRAWROTATEPIC2,
	CG_R_LA_GOGGLES,
	CG_R_SCISSOR,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_SETUSERCMDANGLES,
	CG_S_UPDATEAMBIENTSET,
	CG_S_ADDLOCALSET,
	CG_AS_PARSESETS,
	CG_AS_ADDENTRY,
	CG_AS_GETBMODELSOUND,
	CG_S_GETSAMPLELENGTH,
	COM_SETORGANGLES,
/*
Ghoul2 Insert Start
*/
	CG_G2_LISTBONES,
	CG_G2_LISTSURFACES,
	CG_G2_HAVEWEGHOULMODELS,
	CG_G2_SETMODELS,
/*
Ghoul2 Insert End
*/

	CG_R_GET_LIGHT_STYLE,
	CG_R_SET_LIGHT_STYLE,
	CG_R_GET_BMODEL_VERTS,
	CG_R_WORLD_EFFECT_COMMAND,

	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,
	CG_Z_MALLOC,
	CG_Z_FREE,
	CG_UI_MENU_RESET,
	CG_UI_MENU_NEW,
	CG_UI_PARSE_INT,
	CG_UI_PARSE_STRING,
	CG_UI_PARSE_FLOAT,
	CG_UI_STARTPARSESESSION,
	CG_UI_ENDPARSESESSION,
	CG_UI_PARSEEXT,
	CG_UI_MENUPAINT_ALL,
	CG_UI_STRING_INIT,
	CG_UI_GETMENUINFO,
	CG_SP_REGISTER,
	CG_SP_GETSTRINGTEXTSTRING,
	CG_SP_GETSTRINGTEXT,
	CG_UI_GETITEMTEXT,
	CG_ANYLANGUAGE_READFROMSTRING2,

	CG_OPENJK_MENU_PAINT,
	CG_OPENJK_GETMENU_BYNAME,
} cgameImport_t;

//----------------------------------------------
