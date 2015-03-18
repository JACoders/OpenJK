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

#define ITEM_TYPE_TEXT				0		// simple text
#define ITEM_TYPE_BUTTON			1		// button, basically text with a border 
#define ITEM_TYPE_RADIOBUTTON		2		// toggle button, may be grouped 
#define ITEM_TYPE_CHECKBOX			3		// check box
#define ITEM_TYPE_EDITFIELD			4		// editable text, associated with a cvar
#define ITEM_TYPE_COMBO				5		// drop down list
#define ITEM_TYPE_LISTBOX			6		// scrollable list  
#define ITEM_TYPE_MODEL				7		// model
#define ITEM_TYPE_OWNERDRAW			8		// owner draw, name specs what it is
#define ITEM_TYPE_NUMERICFIELD		9		// editable text, associated with a cvar
#define ITEM_TYPE_SLIDER			10		// mouse speed, volume, etc.
#define ITEM_TYPE_YESNO				11		// yes no cvar setting
#define ITEM_TYPE_MULTI				12		// multiple list setting, enumerated
#define ITEM_TYPE_BIND				13		// multiple list setting, enumerated
#define ITEM_TYPE_TEXTSCROLL		14		// scrolling text


#define ITEM_ALIGN_LEFT 0                 // left alignment
#define ITEM_ALIGN_CENTER 1               // center alignment
#define ITEM_ALIGN_RIGHT 2                // right alignment

#define ITEM_TEXTSTYLE_NORMAL 0           // normal text
#define ITEM_TEXTSTYLE_BLINK 1            // fast blinking
#define ITEM_TEXTSTYLE_PULSE 2            // slow pulsing
#define ITEM_TEXTSTYLE_SHADOWED 3         // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINED 4         // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINESHADOWED 5  // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_SHADOWEDMORE 6         // drop shadow ( need a color for this )
                          
#define WINDOW_BORDER_NONE 0              // no border
#define WINDOW_BORDER_FULL 1              // full border based on border color ( single pixel )
#define WINDOW_BORDER_HORZ 2              // horizontal borders only
#define WINDOW_BORDER_VERT 3              // vertical borders only 
#define WINDOW_BORDER_KCGRADIENT 4        // horizontal border using the gradient bars
  
#define WINDOW_STYLE_EMPTY 0              // no background
#define WINDOW_STYLE_FILLED 1             // filled with background color
#define WINDOW_STYLE_GRADIENT 2           // gradient bar based on background color 
#define WINDOW_STYLE_SHADER   3           // gradient bar based on background color 
#define WINDOW_STYLE_TEAMCOLOR 4          // team color
#define WINDOW_STYLE_CINEMATIC 5          // cinematic

#define MENU_TRUE 1                       // uh.. true
#define MENU_FALSE 0                      // and false

#define HUD_VERTICAL				0x00
#define HUD_HORIZONTAL				0x01

// list box element types
#define LISTBOX_TEXT  0x00
#define LISTBOX_IMAGE 0x01

// list feeders
#define FEEDER_SAVEGAMES					0x00			// save games
#define FEEDER_MAPS							0x01			// text maps based on game type
#define FEEDER_SERVERS						0x02			// servers
#define FEEDER_CLANS						0x03			// clan names
#define FEEDER_ALLMAPS						0x04			// all maps available, in graphic format
#define FEEDER_REDTEAM_LIST					0x05			// red team members
#define FEEDER_BLUETEAM_LIST				0x06			// blue team members
#define FEEDER_PLAYER_LIST					0x07			// players
#define FEEDER_TEAM_LIST					0x08			// team members for team voting
#define FEEDER_MODS							0x09			// 
#define FEEDER_DEMOS 						0x0a			// 
#define FEEDER_SCOREBOARD					0x0b			// 
#define FEEDER_Q3HEADS		 				0x0c			// model heads
#define FEEDER_SERVERSTATUS					0x0d			// server status
#define FEEDER_FINDPLAYER					0x0e			// find player
#define FEEDER_CINEMATICS					0x0f			// cinematics
#define FEEDER_PLAYER_SPECIES				0x10			// models/player/*w
#define FEEDER_PLAYER_SKIN_HEAD				0x11			// head*.skin files in species folder
#define FEEDER_PLAYER_SKIN_TORSO			0x12			// torso*.skin files in species folder
#define FEEDER_PLAYER_SKIN_LEGS				0x13			// lower*.skin files in species folder
#define FEEDER_COLORCHOICES					0x14			// special hack to feed text/actions from playerchoice.txt in species folder
#define FEEDER_MOVES						0x15			// moves for the data pad moves screen
#define FEEDER_MOVES_TITLES					0x16			// move titles for the data pad moves screen
#define FEEDER_LANGUAGES					0x17			// the list of languages 


#define UI_VERSION				200
#define UI_HANDICAP				200
#define UI_EFFECTS				201
#define UI_PLAYERMODEL			202
#define UI_DATAPAD_MISSION		203
#define UI_DATAPAD_WEAPONS		204
#define UI_DATAPAD_INVENTORY	205
#define UI_DATAPAD_FORCEPOWERS	206
#define UI_SKILL				207
#define UI_BLUETEAMNAME			208
#define UI_REDTEAMNAME			209
#define UI_BLUETEAM1			210
#define UI_BLUETEAM2			211
#define UI_BLUETEAM3			212
#define UI_BLUETEAM4			213
#define UI_BLUETEAM5			214
#define UI_REDTEAM1				215
#define UI_REDTEAM2				216
#define UI_REDTEAM3				217
#define UI_REDTEAM4				218
#define UI_REDTEAM5				219
#define UI_NETSOURCE			220
#define UI_NETMAPPREVIEW		221
#define UI_NETFILTER			222
#define UI_TIER					223
#define UI_OPPONENTMODEL		224
#define UI_TIERMAP1				225
#define UI_TIERMAP2				226
#define UI_TIERMAP3				227
#define UI_PLAYERLOGO			228
#define UI_OPPONENTLOGO			229
#define UI_PLAYERLOGO_METAL		230
#define UI_OPPONENTLOGO_METAL	231
#define UI_PLAYERLOGO_NAME		232
#define UI_OPPONENTLOGO_NAME	233
#define UI_TIER_MAPNAME			234
#define UI_TIER_GAMETYPE		235
#define UI_ALLMAPS_SELECTION	236
#define UI_OPPONENT_NAME		237
#define UI_VOTE_KICK			238
#define UI_BOTNAME				239
#define UI_BOTSKILL				240
#define UI_REDBLUE				241
#define UI_CROSSHAIR			242
#define UI_SELECTEDPLAYER		243
#define UI_MAPCINEMATIC			244
#define UI_NETGAMETYPE			245
#define UI_NETMAPCINEMATIC		246
#define UI_SERVERREFRESHDATE	247
#define UI_SERVERMOTD			248
#define UI_GLINFO				249
#define UI_KEYBINDSTATUS		250
#define UI_CLANCINEMATIC		251
#define UI_MAP_TIMETOBEAT		252
#define UI_JOINGAMETYPE			253
#define UI_PREVIEWCINEMATIC		254
#define UI_STARTMAPCINEMATIC	255
#define UI_MAPS_SELECTION		256
