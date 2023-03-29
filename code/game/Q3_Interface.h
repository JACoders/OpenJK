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

#ifndef __Q3_INTERFACE__
#define __Q3_INTERFACE__

#include <array>
#include "../icarus/IcarusInterface.h"
#include "bg_public.h"
#include "g_shared.h"
//NOTENOTE: The enums and tables in this file will obviously bitch if they are included multiple times, don't do that

typedef enum //# setType_e
{
	//# #sep Parm strings
	SET_PARM1 = 0,//## %s="" # Set entity parm1
	SET_PARM2,//## %s="" # Set entity parm2
	SET_PARM3,//## %s="" # Set entity parm3
	SET_PARM4,//## %s="" # Set entity parm4
	SET_PARM5,//## %s="" # Set entity parm5
	SET_PARM6,//## %s="" # Set entity parm6
	SET_PARM7,//## %s="" # Set entity parm7
	SET_PARM8,//## %s="" # Set entity parm8
	SET_PARM9,//## %s="" # Set entity parm9
	SET_PARM10,//## %s="" # Set entity parm10
	SET_PARM11,//## %s="" # Set entity parm11
	SET_PARM12,//## %s="" # Set entity parm12
	SET_PARM13,//## %s="" # Set entity parm13
	SET_PARM14,//## %s="" # Set entity parm14
	SET_PARM15,//## %s="" # Set entity parm15
	SET_PARM16,//## %s="" # Set entity parm16

	// NOTE!!! If you add any other SET_xxxxxxSCRIPT types, make sure you update the 'case' statements in
	//	ICARUS_InterrogateScript() (game/g_ICARUS.cpp), or the script-precacher won't find them.

	//# #sep Scripts and other file paths
	SET_SPAWNSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when spawned //0 - do not change these, these are equal to BSET_SPAWN, etc
	SET_USESCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when used
	SET_AWAKESCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when startled
	SET_ANGERSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script run when find an enemy for the first time
	SET_ATTACKSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when you shoot
	SET_VICTORYSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when killed someone
	SET_LOSTENEMYSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when you can't find your enemy
	SET_PAINSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when hit
	SET_FLEESCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when hit and low health
	SET_DEATHSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when killed
	SET_DELAYEDSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run after a delay
	SET_BLOCKEDSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when blocked by teammate
	SET_FFIRESCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player has shot own team repeatedly
	SET_FFDEATHSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player kills a teammate
	SET_MINDTRICKSCRIPT,//## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player kills a teammate
	SET_VIDEO_PLAY,//## %s="filename" !!"W:\game\base\video\!!#*.roq" # Play a video (inGame)
	SET_CINEMATIC_SKIPSCRIPT, //## %s="filename" !!"W:\game\base\scripts\!!#*.txt" # Script to run when skipping the running cinematic
	SET_RAILCENTERTRACKLOCKED, //## %s="targetname"  # Turn off the centered movers on the given track
	SET_RAILCENTERTRACKUNLOCKED, //## %s="targetname"  # Turn on the centered movers on the given track
	SET_SKIN,//## %s="models/players/???/model_default.skin" # just blindly sets whatever skin you set!  include full path after "base/"... eg: "models/players/tavion_new/model_possessed.skin"

	//# #sep Standard strings
	SET_ENEMY,//## %s="NULL" # Set enemy by targetname
	SET_LEADER,//## %s="NULL" # Set for BS_FOLLOW_LEADER
	SET_NAVGOAL,//## %s="NULL" # *Move to this navgoal then continue script
	SET_CAPTURE,//## %s="NULL" # Set captureGoal by targetname
	SET_VIEWTARGET,//## %s="NULL" # Set angles toward ent by targetname
	SET_WATCHTARGET,//## %s="NULL" # Set angles toward ent by targetname, will *continue* to face them... only in BS_CINEMATIC
	SET_TARGETNAME,//## %s="NULL" # Set/change your targetname
	SET_PAINTARGET,//## %s="NULL" # Set/change what to use when hit
	SET_CAMERA_GROUP,//## %s="NULL" # all ents with this cameraGroup will be focused on
	SET_CAMERA_GROUP_TAG,//## %s="NULL" # What tag on all clients to try to track
	SET_LOOK_TARGET,//## %s="NULL" # object for NPC to look at
	SET_ADDRHANDBOLT_MODEL,			//## %s="NULL" # object to place on NPC right hand bolt
	SET_REMOVERHANDBOLT_MODEL,		//## %s="NULL" # object to remove from NPC right hand bolt
	SET_ADDLHANDBOLT_MODEL,			//## %s="NULL" # object to place on NPC left hand bolt
	SET_REMOVELHANDBOLT_MODEL,		//## %s="NULL" # object to remove from NPC left hand bolt
	SET_CAPTIONTEXTCOLOR,	//## %s=""  # Color of text RED,WHITE,BLUE, YELLOW
	SET_CENTERTEXTCOLOR,	//## %s=""  # Color of text RED,WHITE,BLUE, YELLOW
	SET_SCROLLTEXTCOLOR,	//## %s=""  # Color of text RED,WHITE,BLUE, YELLOW
	SET_COPY_ORIGIN,//## %s="targetname"  # Copy the origin of the ent with targetname to your origin
	SET_DEFEND_TARGET,//## %s="targetname"  # This NPC will attack the target NPC's enemies
	SET_TARGET,//## %s="NULL" # Set/change your target
	SET_TARGET2,//## %s="NULL" # Set/change your target2, on NPC's, this fires when they're knocked out by the red hypo
	SET_LOCATION,//## %s="INVALID" # What trigger_location you're in - Can only be gotten, not set!
	SET_REMOVE_TARGET,//## %s="NULL" # Target that is fired when someone completes the BS_REMOVE behaviorState
	SET_LOADGAME,//## %s="exitholodeck" # Load the savegame that was auto-saved when you started the holodeck
	SET_LOCKYAW,//## %s="off"  # Lock legs to a certain yaw angle (or "off" or "auto" uses current)
	SET_VIEWENTITY,//## %s="NULL" # Make the player look through this ent's eyes - also shunts player movement control to this ent
	SET_LOOPSOUND,//## %s="FILENAME" !!"W:\game\base\!!#sound\*.*" # Looping sound to play on entity
	SET_ICARUS_FREEZE,//## %s="NULL" # Specify name of entity to freeze - !!!NOTE!!! since the ent is frozen, you must have some other entity unfreeze it!!!
	SET_ICARUS_UNFREEZE,//## %s="NULL" # Specify name of entity to unfreeze
	SET_SABER1,//## %s="none" # Name of a saber in sabers.cfg to use in first hand.  "none" removes current saber
	SET_SABER2,//## %s="none" # Name of a saber in sabers.cfg to use in second hand.  "none" removes current saber
	SET_PLAYERMODEL,//## %s="Kyle" # Name of an NPC config in NPC2.cfg to use for this ent
	SET_VEHICLE,//## %s="speeder" # Name of an vehicle config in vehicles.cfg to make this ent drive
	SET_SECURITY_KEY,// %s="keyname" # name of a security key to give to the player - don't place one in map, just give the name here and it handles the rest (use "null" to remove their current key)

	SET_SCROLLTEXT,	//## %s="" # key of text string to print
	SET_LCARSTEXT,	//## %s="" # key of text string to print in LCARS frame
	SET_CENTERTEXT,	//## %s="" # key of text string to print in center of screen.

	//# #sep vectors
	SET_ORIGIN,//## %v="0.0 0.0 0.0" # Set origin explicitly or with TAG
	SET_ANGLES,//## %v="0.0 0.0 0.0" # Set angles explicitly or with TAG
	SET_TELEPORT_DEST,//## %v="0.0 0.0 0.0" # Set origin here as soon as the area is clear
	SET_SABER_ORIGIN,//## %v="0.0 0.0 0.0" # Removes this ent's saber from their hand, turns it off, and places it at the specified location

	//# #sep floats
	SET_XVELOCITY,//## %f="0.0" # Velocity along X axis
	SET_YVELOCITY,//## %f="0.0" # Velocity along Y axis
	SET_ZVELOCITY,//## %f="0.0" # Velocity along Z axis
	SET_Z_OFFSET,//## %f="0.0" # Vertical offset from original origin... offset/ent's speed * 1000ms is duration
	SET_DPITCH,//## %f="0.0" # Pitch for NPC to turn to
	SET_DYAW,//## %f="0.0" # Yaw for NPC to turn to
	SET_TIMESCALE,//## %f="0.0" # Speed-up slow down game (0 - 1.0)
	SET_CAMERA_GROUP_Z_OFS,//## %s="NULL" # when following an ent with the camera, apply this z ofs
	SET_VISRANGE,//## %f="0.0" # How far away NPC can see
	SET_EARSHOT,//## %f="0.0" # How far an NPC can hear
	SET_VIGILANCE,//## %f="0.0" # How often to look for enemies (0 - 1.0)
	SET_GRAVITY,//## %f="0.0" # Change this ent's gravity - 800 default
	SET_FACEAUX,		//## %f="0.0" # Set face to Aux expression for number of seconds
	SET_FACEBLINK,		//## %f="0.0" # Set face to Blink expression for number of seconds
	SET_FACEBLINKFROWN,	//## %f="0.0" # Set face to Blinkfrown expression for number of seconds
	SET_FACEFROWN,		//## %f="0.0" # Set face to Frown expression for number of seconds
	SET_FACESMILE,		//## %f="0.0" # Set face to Smile expression for number of seconds
	SET_FACEGLAD,		//## %f="0.0" # Set face to Glad expression for number of seconds
	SET_FACEHAPPY,		//## %f="0.0" # Set face to Happy expression for number of seconds
	SET_FACESHOCKED,		//## %f="0.0" # Set face to Shocked expression for number of seconds
	SET_FACENORMAL,		//## %f="0.0" # Set face to Normal expression for number of seconds
	SET_FACEEYESCLOSED,	//## %f="0.0" # Set face to Eyes closed
	SET_FACEEYESOPENED,	//## %f="0.0" # Set face to Eyes open
	SET_WAIT,		//## %f="0.0" # Change an entity's wait field
	SET_FOLLOWDIST,		//## %f="0.0" # How far away to stay from leader in BS_FOLLOW_LEADER
	SET_SCALE,			//## %f="0.0" # Scale the entity model
	SET_RENDER_CULL_RADIUS,			//## %f="40.0" # Used to ensure rendering for entities with geographically sprawling animations (world units)
	SET_DISTSQRD_TO_PLAYER,			//## %f="0.0" # Only to be used in a 'get'. (Distance to player)*(Distance to player)

	//# #sep ints
	SET_ANIM_HOLDTIME_LOWER,//## %d="0" # Hold lower anim for number of milliseconds
	SET_ANIM_HOLDTIME_UPPER,//## %d="0" # Hold upper anim for number of milliseconds
	SET_ANIM_HOLDTIME_BOTH,//## %d="0" # Hold lower and upper anims for number of milliseconds
	SET_HEALTH,//## %d="0" # Change health
	SET_ARMOR,//## %d="0" # Change armor
	SET_WALKSPEED,//## %d="0" # Change walkSpeed
	SET_RUNSPEED,//## %d="0" # Change runSpeed
	SET_YAWSPEED,//## %d="0" # Change yawSpeed
	SET_AGGRESSION,//## %d="0" # Change aggression 1-5
	SET_AIM,//## %d="0" # Change aim 1-5
	SET_FRICTION,//## %d="0" # Change ent's friction - 6 default
	SET_SHOOTDIST,//## %d="0" # How far the ent can shoot - 0 uses weapon
	SET_HFOV,//## %d="0" # Horizontal field of view
	SET_VFOV,//## %d="0" # Vertical field of view
	SET_DELAYSCRIPTTIME,//## %d="0" # How many milliseconds to wait before running delayscript
	SET_FORWARDMOVE,//## %d="0" # NPC move forward -127(back) to 127
	SET_RIGHTMOVE,//## %d="0" # NPC move right -127(left) to 127
	SET_STARTFRAME,	//## %d="0" # frame to start animation sequence on
	SET_ENDFRAME,	//## %d="0" # frame to end animation sequence on
	SET_ANIMFRAME,	//## %d="0" # frame to set animation sequence to
	SET_COUNT,	//## %d="0" # Change an entity's count field
	SET_SHOT_SPACING,//## %d="1000" # Time between shots for an NPC - reset to defaults when changes weapon
	SET_MISSIONSTATUSTIME,//## %d="0" # Amount of time until Mission Status should be shown after death
	SET_WIDTH,//## %d="0.0" # Width of NPC bounding box.
	SET_SABER1BLADEON,//## %d="0.0" # Activate a specific blade of Saber 1 (0 - (MAX_BLADES - 1)).
	SET_SABER1BLADEOFF,//## %d="0.0" # Deactivate a specific blade of Saber 1 (0 - (MAX_BLADES - 1)).
	SET_SABER2BLADEON,//## %d="0.0" # Activate a specific blade of Saber 2 (0 - (MAX_BLADES - 1)).
	SET_SABER2BLADEOFF,//## %d="0.0" # Deactivate a specific blade of Saber 2 (0 - (MAX_BLADES - 1)).
	SET_DAMAGEENTITY,	//## %d="5" # Damage this entity with set amount.

	//# #sep booleans
	SET_IGNOREPAIN,//## %t="BOOL_TYPES" # Do not react to pain
	SET_IGNOREENEMIES,//## %t="BOOL_TYPES" # Do not acquire enemies
	SET_IGNOREALERTS,//## %t="BOOL_TYPES" # Do not get enemy set by allies in area(ambush)
	SET_DONTSHOOT,//## %t="BOOL_TYPES" # Others won't shoot you
	SET_NOTARGET,//## %t="BOOL_TYPES" # Others won't pick you as enemy
	SET_DONTFIRE,//## %t="BOOL_TYPES" # Don't fire your weapon
	SET_LOCKED_ENEMY,//## %t="BOOL_TYPES" # Keep current enemy until dead
	SET_CROUCHED,//## %t="BOOL_TYPES" # Force NPC to crouch
	SET_WALKING,//## %t="BOOL_TYPES" # Force NPC to move at walkSpeed
	SET_RUNNING,//## %t="BOOL_TYPES" # Force NPC to move at runSpeed
	SET_CHASE_ENEMIES,//## %t="BOOL_TYPES" # NPC will chase after enemies
	SET_LOOK_FOR_ENEMIES,//## %t="BOOL_TYPES" # NPC will be on the lookout for enemies
	SET_FACE_MOVE_DIR,//## %t="BOOL_TYPES" # NPC will face in the direction it's moving
	SET_DONT_FLEE,//## %t="BOOL_TYPES" # NPC will not run from danger
	SET_FORCED_MARCH,//## %t="BOOL_TYPES" # NPC will not move unless you aim at him
	SET_UNDYING,//## %t="BOOL_TYPES" # Can take damage down to 1 but not die
	SET_NOAVOID,//## %t="BOOL_TYPES" # Will not avoid other NPCs or architecture
	SET_SOLID,//## %t="BOOL_TYPES" # Make yourself notsolid or solid
	SET_PLAYER_USABLE,//## %t="BOOL_TYPES" # Can be activateby the player's "use" button
	SET_LOOP_ANIM,//## %t="BOOL_TYPES" # For non-NPCs, loop your animation sequence
	SET_INTERFACE,//## %t="BOOL_TYPES" # Player interface on/off
	SET_SHIELDS,//## %t="BOOL_TYPES" # NPC has no shields (Borg do not adapt)
	SET_INVISIBLE,//## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
	SET_VAMPIRE,//## %t="BOOL_TYPES" # Draws only in mirrors/portals
	SET_FORCE_INVINCIBLE,//## %t="BOOL_TYPES" # Force Invincibility effect, also godmode
	SET_GREET_ALLIES,//## %t="BOOL_TYPES" # Makes an NPC greet teammates
	SET_VIDEO_FADE_IN,//## %t="BOOL_TYPES" # Makes video playback fade in
	SET_VIDEO_FADE_OUT,//## %t="BOOL_TYPES" # Makes video playback fade out
	SET_PLAYER_LOCKED,//## %t="BOOL_TYPES" # Makes it so player cannot move
	SET_LOCK_PLAYER_WEAPONS,//## %t="BOOL_TYPES" # Makes it so player cannot switch weapons
	SET_NO_IMPACT_DAMAGE,//## %t="BOOL_TYPES" # Stops this ent from taking impact damage
	SET_NO_KNOCKBACK,//## %t="BOOL_TYPES" # Stops this ent from taking knockback from weapons
	SET_ALT_FIRE,//## %t="BOOL_TYPES" # Force NPC to use altfire when shooting
	SET_NO_RESPONSE,//## %t="BOOL_TYPES" # NPCs will do generic responses when this is on (usescripts override generic responses as well)
	SET_INVINCIBLE,//## %t="BOOL_TYPES" # Completely unkillable
	SET_MISSIONSTATUSACTIVE,	//# Turns on Mission Status Screen
	SET_NO_COMBAT_TALK,//## %t="BOOL_TYPES" # NPCs will not do their combat talking noises when this is on
	SET_NO_ALERT_TALK,//## %t="BOOL_TYPES" # NPCs will not do their combat talking noises when this is on
	SET_TREASONED,//## %t="BOOL_TYPES" # Player has turned on his own- scripts will stop, NPCs will turn on him and level changes load the brig
	SET_DISABLE_SHADER_ANIM,//## %t="BOOL_TYPES" # Allows turning off an animating shader in a script
	SET_SHADER_ANIM,//## %t="BOOL_TYPES" # Sets a shader with an image map to be under frame control
	SET_SABERACTIVE,//## %t="BOOL_TYPES" # Turns saber on/off
	SET_ADJUST_AREA_PORTALS,//## %t="BOOL_TYPES" # Only set this on things you move with script commands that you *want* to open/close area portals.  Default is off.
	SET_DMG_BY_HEAVY_WEAP_ONLY,//## %t="BOOL_TYPES" # When true, only a heavy weapon class missile/laser can damage this ent.
	SET_SHIELDED,//## %t="BOOL_TYPES" # When true, ion_cannon is shielded from any kind of damage.
	SET_NO_GROUPS,//## %t="BOOL_TYPES" # This NPC cannot alert groups or be part of a group
	SET_FIRE_WEAPON,//## %t="BOOL_TYPES" # Makes NPC will hold down the fire button, until this is set to false
	SET_FIRE_WEAPON_NO_ANIM,//## %t="BOOL_TYPES" # NPC will hold down the fire button, but they won't play firing anim
	SET_SAFE_REMOVE,//## %t="BOOL_TYPES" # NPC will remove only when it's safe (Player is not in PVS)
	SET_BOBA_JET_PACK,//## %t="BOOL_TYPES" # Turn on/off Boba Fett's Jet Pack
	SET_NO_MINDTRICK,//## %t="BOOL_TYPES" # Makes NPC immune to jedi mind-trick
	SET_INACTIVE,//## %t="BOOL_TYPES" # in lieu of using a target_activate or target_deactivate
	SET_FUNC_USABLE_VISIBLE,//## %t="BOOL_TYPES" # provides an alternate way of changing func_usable to be visible or not, DOES NOT AFFECT SOLID
	SET_SECRET_AREA_FOUND,//## %t="BOOL_TYPES" # Increment secret areas found counter
	SET_END_SCREENDISSOLVE,//## %t="BOOL_TYPES" # End of game dissolve into star background and credits
	SET_USE_CP_NEAREST,//## %t="BOOL_TYPES" # NPCs will use their closest combat points, not try and find ones next to the player, or flank player
	SET_MORELIGHT,//## %t="BOOL_TYPES" # NPC will have a minlight of 96
	SET_NO_FORCE,//## %t="BOOL_TYPES" # NPC will not be affected by force powers
	SET_NO_FALLTODEATH,//## %t="BOOL_TYPES" # NPC will not scream and tumble and fall to hit death over large drops
	SET_DISMEMBERABLE,//## %t="BOOL_TYPES" # NPC will not be dismemberable if you set this to false (default is true)
	SET_NO_ACROBATICS,//## %t="BOOL_TYPES" # Jedi won't jump, roll or cartwheel
	SET_USE_SUBTITLES,//## %t="BOOL_TYPES" # When true NPC will always display subtitle regardless of subtitle setting
	SET_CLEAN_DAMAGING_ENTS,//## %t="BOOL_TYPES" # Removes entities that could muck up cinematics, explosives, turrets, seekers.
	SET_HUD,//## %t="BOOL_TYPES" # Turns on/off HUD
	//JKA
	SET_NO_PVS_CULL,//## %t="BOOL_TYPES" # This entity will *always* be drawn - use only for special case cinematic NPCs that have anims that cover multiple rooms!!!
	SET_CLOAK,		//## %t="BOOL_TYPES" # Set a Saboteur to cloak (true) or un-cloak (false).
	SET_FORCE_HEAL,//## %t="BOOL_TYPES" # Causes this ent to start force healing at whatever level of force heal they have
	SET_FORCE_SPEED,//## %t="BOOL_TYPES" # Causes this ent to start force speeding at whatever level of force speed they have (may not do anything for NPCs?)
	SET_FORCE_PUSH,//## %t="BOOL_TYPES" # Causes this ent to do a force push at whatever level of force push they have - will not fail
	SET_FORCE_PUSH_FAKE,//## %t="BOOL_TYPES" # Causes this ent to do a force push anim, sound and effect, will not push anything
	SET_FORCE_PULL,//## %t="BOOL_TYPES" # Causes this ent to do a force push at whatever level of force push they have - will not fail
	SET_FORCE_MIND_TRICK,//## %t="BOOL_TYPES" # Causes this ent to do a jedi mind trick at whatever level of mind trick they have (may not do anything for NPCs?)
	SET_FORCE_GRIP,//## %t="BOOL_TYPES" # Causes this ent to grip their enemy at whatever level of grip they have (will grip until scripted to stop)
	SET_FORCE_LIGHTNING,//## %t="BOOL_TYPES" # Causes this ent to lightning at whatever level of lightning they have (will lightning until scripted to stop)
	SET_FORCE_SABERTHROW,//## %t="BOOL_TYPES" # Causes this ent to throw their saber at whatever level of saber throw they have (will throw saber until scripted to stop)
	SET_FORCE_RAGE,//## %t="BOOL_TYPES" # Causes this ent to go into force rage at whatever level of force rage they have
	SET_FORCE_PROTECT,//## %t="BOOL_TYPES" # Causes this ent to start a force protect at whatever level of force protect they have
	SET_FORCE_ABSORB,//## %t="BOOL_TYPES" # Causes this ent to do start a force absorb at whatever level of force absorb they have
	SET_FORCE_DRAIN,//## %t="BOOL_TYPES" # Causes this ent to start force draining their enemy at whatever level of force drain they have (will drain until scripted to stop)
	SET_WINTER_GEAR, //## %t="BOOL_TYPES" # Set the player to wear his/her winter gear (skins torso_g1 and lower_e1), or restore the default skins.
	SET_NO_ANGLES, //## %t="BOOL_TYPES" # This NPC/player will not have any bone angle overrides or pitch or roll (should only be used in cinematics)

	//# #sep calls
	SET_SKILL,//## %r%d="0" # Cannot set this, only get it - valid values are 0 through 3

	//# #sep Special tables
	SET_ANIM_UPPER,//## %t="ANIM_NAMES" # Torso and head anim
	SET_ANIM_LOWER,//## %t="ANIM_NAMES" # Legs anim
	SET_ANIM_BOTH,//## %t="ANIM_NAMES" # Set same anim on torso and legs
	SET_PLAYER_TEAM,//## %t="TEAM_NAMES" # Your team
	SET_ENEMY_TEAM,//## %t="TEAM_NAMES" # Team in which to look for enemies
	SET_BEHAVIOR_STATE,//## %t="BSTATE_STRINGS" # Change current bState
	SET_DEFAULT_BSTATE,//## %t="BSTATE_STRINGS" # Change fallback bState
	SET_TEMP_BSTATE,//## %t="BSTATE_STRINGS" # Set/Chang a temp bState
	SET_EVENT,//## %t="EVENT_NAMES" # Events you can initiate
	SET_WEAPON,//## %t="WEAPON_NAMES" # Change/Stow/Drop weapon
	SET_ITEM,//## %t="ITEM_NAMES" # Give items
	SET_MUSIC_STATE,//## %t="MUSIC_STATES" # Set the state of the dynamic music

	SET_FORCE_HEAL_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_JUMP_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_SPEED_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_PUSH_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_PULL_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_MINDTRICK_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_GRIP_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_LIGHTNING_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_SABER_THROW,//## %t="FORCE_LEVELS" # Change force power level
	SET_SABER_DEFENSE,//## %t="FORCE_LEVELS" # Change force power level
	SET_SABER_OFFENSE,//## %t="SABER_STYLES" # Change force power level
	SET_FORCE_RAGE_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_PROTECT_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_ABSORB_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_DRAIN_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_FORCE_SIGHT_LEVEL,//## %t="FORCE_LEVELS" # Change force power level
	SET_SABER1_COLOR1,		//## %t="SABER_COLORS" # Set color of first blade of first saber
	SET_SABER1_COLOR2,		//## %t="SABER_COLORS" # Set color of second blade of first saber
	SET_SABER2_COLOR1,		//## %t="SABER_COLORS" # Set color of first blade of first saber
	SET_SABER2_COLOR2,		//## %t="SABER_COLORS" # Set color of second blade of first saber
	SET_DISMEMBER_LIMB,		//## %t="HIT_LOCATIONS" # Cut off a part of a body and send the limb flying

	SET_OBJECTIVE_SHOW,	//## %t="OBJECTIVES" # Show objective on mission screen
	SET_OBJECTIVE_HIDE,	//## %t="OBJECTIVES" # Hide objective from mission screen
	SET_OBJECTIVE_SUCCEEDED,//## %t="OBJECTIVES" # Mark objective as completed
	SET_OBJECTIVE_SUCCEEDED_NO_UPDATE,//## %t="OBJECTIVES" # Mark objective as completed, no update sent to screen
	SET_OBJECTIVE_FAILED,	//## %t="OBJECTIVES" # Mark objective as failed

	SET_MISSIONFAILED,		//## %t="MISSIONFAILED" # Mission failed screen activates

	SET_TACTICAL_SHOW,		//## %t="TACTICAL" # Show tactical info on mission objectives screen
	SET_TACTICAL_HIDE,		//## %t="TACTICAL" # Hide tactical info on mission objectives screen
	SET_OBJECTIVE_CLEARALL,	//## # Force all objectives to be hidden
/*
	SET_OBJECTIVEFOSTER,
*/
	SET_OBJECTIVE_LIGHTSIDE,	//## # Used to get whether the player has chosen the light (succeeded) or dark (failed) side.

	SET_MISSIONSTATUSTEXT,	//## %t="STATUSTEXT" # Text to appear in mission status screen
	SET_MENU_SCREEN,//## %t="MENUSCREENS" # Brings up specified menu screen

	SET_CLOSINGCREDITS,		//## # Show closing credits

	//in-bhc tables
	SET_LEAN,//## %t="LEAN_TYPES" # Lean left, right or stop leaning

	//# #eol
	SET_
} setType_t;


// this enum isn't used directly by the game, it's mainly for BehavEd to scan for...
//
typedef enum //# playType_e
{
	//# #sep Types of file to play
	PLAY_ROFF = 0,//## %s="filename" !!"W:\game\base\scripts\!!#*.rof" # Play a ROFF file

	//# #eol
	PLAY_NUMBEROF

} playType_t;


const	int	Q3_TIME_SCALE	= 1;	//MILLISECONDS

extern char	cinematicSkipScript[64];

//General
extern	void		Q3_TaskIDClear( int *taskID );
extern	qboolean	Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern	void		Q3_TaskIDComplete( gentity_t *ent, taskID_t taskType );
extern	void		Q3_DPrintf( const char *, ... );

//Not referenced directly as script function - all are called through Q3_Set
extern	void		Q3_SetAnimBoth( int entID, const char *anim_name );
extern	void		Q3_SetVelocity( int entID, vec3_t angles );

//////////////////////////////////////////////////////////////////////////
/*		BEGIN Almost useless tokenizer and interpreter constants BEGIN	*/
//////////////////////////////////////////////////////////////////////////
#define MAX_STRING_LENGTH		256
#define MAX_IDENTIFIER_LENGTH	128

#define TKF_IGNOREDIRECTIVES			0x00000001		// skip over lines starting with #
#define TKF_USES_EOL					0x00000002		// generate end of line tokens
#define TKF_NODIRECTIVES				0x00000004		// don't treat # in any special way
#define TKF_WANTUNDEFINED				0x00000008		// if token not found in symbols create undefined token
#define TKF_WIDEUNDEFINEDSYMBOLS		0x00000010		// when undefined token encountered, accumulate until space
#define TKF_RAWSYMBOLSONLY				0x00000020
#define TKF_NUMERICIDENTIFIERSTART		0x00000040
#define TKF_IGNOREKEYWORDS				0x00000080
#define TKF_NOCASEKEYWORDS				0x00000100
#define TKF_NOUNDERSCOREINIDENTIFIER	0x00000200
#define TKF_NODASHINIDENTIFIER			0x00000400
#define TKF_COMMENTTOKENS				0x00000800

enum
{
	TKERR_NONE,
	TKERR_UNKNOWN,
	TKERR_BUFFERCREATE,
	TKERR_UNRECOGNIZEDSYMBOL,
	TKERR_DUPLICATESYMBOL,
	TKERR_STRINGLENGTHEXCEEDED,
	TKERR_IDENTIFIERLENGTHEXCEEDED,
	TKERR_EXPECTED_INTEGER,
	TKERR_EXPECTED_IDENTIFIER,
	TKERR_EXPECTED_STRING,
	TKERR_EXPECTED_CHAR,
	TKERR_EXPECTED_FLOAT,
	TKERR_UNEXPECTED_TOKEN,
	TKERR_INVALID_DIRECTIVE,
	TKERR_INCLUDE_FILE_NOTFOUND,
	TKERR_UNMATCHED_DIRECTIVE,
	TKERR_USERERROR,
};

enum
{
	TK_EOF = -1,
	TK_UNDEFINED,
	TK_COMMENT,
	TK_EOL,
	TK_CHAR,
	TK_STRING,
	TK_INT,
	TK_INTEGER = TK_INT,
	TK_FLOAT,
	TK_IDENTIFIER,
	TK_USERDEF,
};

//Token defines
enum
{
	TK_BLOCK_START = TK_USERDEF,
	TK_BLOCK_END,
	TK_VECTOR_START,
	TK_VECTOR_END,
	TK_OPEN_PARENTHESIS,
	TK_CLOSED_PARENTHESIS,
	TK_VECTOR,
	TK_GREATER_THAN,
	TK_LESS_THAN,
	TK_EQUALS,
	TK_NOT,

	NUM_USER_TOKENS
};

//ID defines
enum
{
	ID_AFFECT = NUM_USER_TOKENS,
	ID_SOUND,
	ID_MOVE,
	ID_ROTATE,
	ID_WAIT,
	ID_BLOCK_START,
	ID_BLOCK_END,
	ID_SET,
	ID_LOOP,
	ID_LOOPEND,
	ID_PRINT,
	ID_USE,
	ID_FLUSH,
	ID_RUN,
	ID_KILL,
	ID_REMOVE,
	ID_CAMERA,
	ID_GET,
	ID_RANDOM,
	ID_IF,
	ID_ELSE,
	ID_REM,
	ID_TASK,
	ID_DO,
	ID_DECLARE,
	ID_FREE,
	ID_DOWAIT,
	ID_SIGNAL,
	ID_WAITSIGNAL,
	ID_PLAY,

	ID_TAG,
	ID_EOF,
	NUM_IDS
};

//Type defines
enum
{
	//Wait types
	TYPE_WAIT_COMPLETE	 = NUM_IDS,
	TYPE_WAIT_TRIGGERED,

	//Set types
	TYPE_ANGLES,
	TYPE_ORIGIN,

	//Affect types
	TYPE_INSERT,
	TYPE_FLUSH,

	//Camera types
	TYPE_PAN,
	TYPE_ZOOM,
	TYPE_MOVE,
	TYPE_FADE,
	TYPE_PATH,
	TYPE_ENABLE,
	TYPE_DISABLE,
	TYPE_SHAKE,
	TYPE_ROLL,
	TYPE_TRACK,
	TYPE_DISTANCE,
	TYPE_FOLLOW,

	//Variable type
	TYPE_VARIABLE,

	TYPE_EOF,

	ID_CONTINUE,
	ID_BREAK,
	ID_ACTIVATE,
	ID_DEACTIVATE,
	ID_WHILE,
	ID_WAITUNTIL,
	ID_WAITAFFECT,
	ID_TASKCOMPLETED,
	ID_SIGNALCOUNT,
	TK_GE,
	TK_LE,

	NUM_TYPES
};

enum
{
	MSG_COMPLETED,
	MSG_EOF,
	NUM_MESSAGES,
};
//////////////////////////////////////////////////////////////////////////
/*						END Constants END								*/
//////////////////////////////////////////////////////////////////////////

//NOTENOTE: Only change this to re-point ICARUS to a new script directory
// NOTE! A '/' (forward slash) should be checked for along with the script dir.
#define Q3_SCRIPT_DIR	"scripts"

#define MAX_FILENAME_LENGTH	256

#define	IBI_EXT			".IBI"	//(I)nterpreted (B)lock (I)nstructions
#define IBI_HEADER_ID	"IBI"

//////////////////////////////////////////////////////////////////////////
/*					BEGIN Quake 3 Game Interface BEGIN					*/
//////////////////////////////////////////////////////////////////////////

// The script data buffer.
typedef struct pscript_s
{
	char	*buffer;
	long	length;
} pscript_t;

// STL map type definitions for the Entity List and Script Buffer List.
typedef	std::map < std::string, int >		entitylist_t;
typedef std::map < std::string, pscript_t* >	scriptlist_t;

// STL map type definitions for the variable containers.
typedef std::map < std::string, std::string >		varString_m;
typedef std::map < std::string, float >		varFloat_m;
// For cases where we need to re-use a buffer without invalidating it
typedef std::map < std::string, std::array< char, MAX_STRING_CHARS > >		varStringBuf_m;


// The Quake 3 Game Interface Class for Quake3 and Icarus to use.
// Created: 10/08/02 by Aurelio Reis.
class CQuake3GameInterface : public IGameInterface
{
private:
	// A list of script buffers (to ensure we cache a script only once).
	scriptlist_t		m_ScriptList;

	// A list of Game Elements/Objects.
	entitylist_t		m_EntityList;

	// Variable stuff.
	varString_m		m_varStrings;
	varFloat_m		m_varFloats;
	varString_m		m_varVectors;
	int				m_numVariables;

	varStringBuf_m		m_cvars;

	int				m_entFilter;

	// Register variables functions.
	void SetVar( int taskID, int entID, const char *type_name, const char *data );
	void VariableSaveFloats( varFloat_m &fmap );
	void VariableSaveStrings( varString_m &smap );
	void VariableLoadFloats( varFloat_m &fmap );
	void VariableLoadStrings( int type, varString_m &fmap );
	void InitVariables( void );
	int  GetStringVariable( const char *name, const char **value );
	int  GetFloatVariable( const char *name, float *value );
	int  GetVectorVariable( const char *name, vec3_t value );
	int  VariableDeclared( const char *name );
	int  SetFloatVariable( const char *name, float value );
	int  SetStringVariable( const char *name, const char *value );
	int  SetVectorVariable( const char *name, const char *value );
	void PrisonerObjCheck(const char *name,const char *data);

public:
	// Static Singleton Instance.
	static CQuake3GameInterface *m_pInstance;

	// Variable enums
	enum { VTYPE_NONE = 0, VTYPE_FLOAT, VTYPE_STRING, VTYPE_VECTOR, MAX_VARIABLES = 32 };

	// Register enums.
	enum { SCRIPT_COULDNOTREGISTER = 0, SCRIPT_REGISTERED, SCRIPT_ALREADYREGISTERED };

	// Constructor.
	CQuake3GameInterface();

	// Destructor (NOTE: Destroy the Game Interface BEFORE the Icarus Interface).
	~CQuake3GameInterface();

	// Initialize an Entity by ID.
	bool InitEntity( gentity_t *pEntity );

	// Free an Entity by ID (NOTE, if this is called while a script is running the game will crash!).
	void FreeEntity( gentity_t *pEntity );

	// Determines whether or not an Entity needs ICARUS information.
	bool ValidEntity( gentity_t *pEntity );

	// Associate the entity's id and name so that it can be referenced later.
	void AssociateEntity( gentity_t *pEntity );

	// Make a valid script name.
	int MakeValidScriptName( char **strScriptName );

	// First looks to see if a script has already been loaded, if so, return SCRIPT_ALREADYREGISTERED. If a script has
	// NOT been already cached, that script is loaded and the return is SCRIPT_REGISTERED. If a script could not
	// be found cached and could not be loaded we return SCRIPT_COULDNOTREGISTER.
	int RegisterScript( const char *strFileName, void **ppBuf, int &iLength );

	// Precache all the resources needed by a Script and it's Entity (or vice-versa).
	int PrecacheEntity( gentity_t *pEntity );

	// Run the script.
	void RunScript( const gentity_t *pEntity, const char *strScriptName );

	// Log Icarus Entity's?
	void Svcmd( void );

	// Clear the list of entitys.
	void ClearEntityList() { m_EntityList.clear(); }

	// Save all Variables.
	int VariableSave( void );

	// Load all Variables.
	int VariableLoad( void );

    // Overiddables.

	// Get the current Game flavor.
	int GetFlavor() OVERRIDE;

	//General
	int		LoadFile( const char *name, void **buf ) OVERRIDE;
	void	CenterPrint( const char *format, ... ) OVERRIDE;
	void	DebugPrint( e_DebugPrintLevel, const char *, ... ) OVERRIDE;
	unsigned int GetTime( void ) OVERRIDE;							//Gets the current time
	//DWORD	GetTimeScale(void );
	int 	PlayIcarusSound( int taskID, int entID, const char *name, const char *channel ) OVERRIDE;
	void	Lerp2Pos( int taskID, int entID, vec3_t origin, vec3_t angles, float duration ) OVERRIDE;
	void	Lerp2Angles( int taskID, int entID, vec3_t angles, float duration ) OVERRIDE;
	int		GetTag( int entID, const char *name, int lookup, vec3_t info ) OVERRIDE;
	void	Set( int taskID, int entID, const char *type_name, const char *data ) OVERRIDE;
	void	Use( int entID, const char *name ) OVERRIDE;
	void	Activate( int entID, const char *name ) OVERRIDE;
	void	Deactivate( int entID, const char *name ) OVERRIDE;
	void	Kill( int entID, const char *name ) OVERRIDE;
	void	Remove( int entID, const char *name ) OVERRIDE;
	float	Random( float min, float max ) OVERRIDE;
	void	Play( int taskID, int entID, const char *type, const char *name ) OVERRIDE;

	//Camera functions
	void	CameraPan( vec3_t angles, vec3_t dir, float duration ) OVERRIDE;
	void	CameraMove( vec3_t origin, float duration ) OVERRIDE;
	void	CameraZoom( float fov, float duration ) OVERRIDE;
	void	CameraRoll( float angle, float duration ) OVERRIDE;
	void	CameraFollow( const char *name, float speed, float initLerp ) OVERRIDE;
	void	CameraTrack( const char *name, float speed, float initLerp ) OVERRIDE;
	void	CameraDistance( float dist, float initLerp ) OVERRIDE;
	void	CameraFade( float sr, float sg, float sb, float sa, float dr, float dg, float db, float da, float duration ) OVERRIDE;
	void	CameraPath( const char *name ) OVERRIDE;
	void	CameraEnable( void ) OVERRIDE;
	void	CameraDisable( void ) OVERRIDE;
	void	CameraShake( float intensity, int duration ) OVERRIDE;

	int		GetFloat( int entID, const char *name, float *value ) OVERRIDE;
	int		GetVector( int entID, const char *name, vec3_t value ) OVERRIDE;
	int		GetString( int entID, const char *name, char **value ) OVERRIDE;

	int		Evaluate( int p1Type, const char *p1, int p2Type, const char *p2, int operatorType ) OVERRIDE;

	void	DeclareVariable( int type, const char *name ) OVERRIDE;
	void	FreeVariable( const char *name ) OVERRIDE;

	//Save / Load functions
	int		LinkGame( int entID, int icarusID ) OVERRIDE;

	ojk::ISavedGame* get_saved_game_file() override;

	// Access functions
	int		CreateIcarus( int entID) OVERRIDE;
			//Polls the engine for the sequencer of the entity matching the name passed
	int		GetByName( const char *name ) OVERRIDE;
	// (g_entities[m_ownerID].svFlags&SVF_ICARUS_FREEZE)	// return -1 indicates invalid
	int		IsFrozen(int entID) OVERRIDE;
	void	Free(void* data) OVERRIDE;
	void	*Malloc( int size ) OVERRIDE;
	float	MaxFloat(void) OVERRIDE;

	// Script precache functions.
	void	PrecacheRoff( const char *name ) OVERRIDE;
	void	PrecacheScript( const char *name ) OVERRIDE;
	void	PrecacheSound( const char *name ) OVERRIDE;
	void	PrecacheFromSet( const char *setname, const char *filename ) OVERRIDE;
};

// A Quick accessor function for accessing Quake 3 Interface specific functions.
inline CQuake3GameInterface *Quake3Game() { return (CQuake3GameInterface *)IGameInterface::GetGame(); }

//////////////////////////////////////////////////////////////////////////
/*					END Quake 3 Game Interface END						*/
//////////////////////////////////////////////////////////////////////////

#endif	//__Q3_INTERFACE__
