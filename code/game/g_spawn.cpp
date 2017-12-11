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

#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "g_functions.h"

extern cvar_t *g_spskill;
extern cvar_t *g_delayedShutdown;

// these vars I moved here out of the level_locals_t struct simply because it's pointless to try saving them,
//	and the level_locals_t struct is included in the save process... -slc
//
qboolean	spawning = qfalse;				// the G_Spawn*() functions are valid  (only turned on during one function)
int			numSpawnVars;
char		*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
int			numSpawnVarChars;
char		spawnVarChars[MAX_SPAWN_VARS_CHARS];

int			delayedShutDown = 0;

#include "../qcommon/sstring.h"

//NOTENOTE: Be sure to change the mirrored code in cgmain.cpp
typedef	std::map< sstring_t, unsigned char  >	namePrecache_m;
namePrecache_m	*as_preCacheMap = NULL;

char *G_AddSpawnVarToken( const char *string );

void AddSpawnField(char *field, char *value)
{
	int	i;

	for(i=0;i<numSpawnVars;i++)
	{
		if (Q_stricmp(spawnVars[i][0], field) == 0)
		{
			spawnVars[ i ][1] = G_AddSpawnVarToken( value );
			return;
		}
	}

	spawnVars[ numSpawnVars ][0] = G_AddSpawnVarToken( field );
	spawnVars[ numSpawnVars ][1] = G_AddSpawnVarToken( value );
	numSpawnVars++;
}

qboolean	G_SpawnField( unsigned int uiField, char **ppKey, char **ppValue )
{
	if ( (int)uiField >= numSpawnVars )
		return qfalse;

	(*ppKey) = spawnVars[uiField][0];
	*ppValue = spawnVars[uiField][1];

	return qtrue;
}

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, spawnVars[i][0] ) ) {
			*out = spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}

qboolean	G_SpawnVector4( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f %f", &out[0], &out[1], &out[2], &out[3] );
	return present;
}

qboolean	G_SpawnFlag( const char *key, int flag, int *out )
{
	//find that key
	for ( int i = 0 ; i < numSpawnVars ; i++ )
	{
		if ( !strcmp( key, spawnVars[i][0] ) )
		{
			//found the key
			if ( atoi( spawnVars[i][1] ) != 0 )
			{//if it's non-zero, and in the flag
				*out |= flag;
			}
			else
			{//if it's zero, or out the flag
				*out &= ~flag;
			}
			return qtrue;
		}
	}

	return qfalse;
}

qboolean G_SpawnAngleHack( const char *key, const char *defaultString, float *out )
{
	char		*s;
	qboolean	present;
	float		temp = 0;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f", &temp );

	out[0] = 0;
	out[1] = temp;
	out[2] = 0;

	return present;
}

stringID_table_t flagTable [] =
{
	//"noTED", EF_NO_TED,
	//stringID_table_t Must end with a null entry
	{ "", 0 }
};

//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_VECTOR4,
	F_ANGLEHACK,
	F_ENTITY,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_PARM1,			// Special case for parms
	F_PARM2,			// Special case for parms
	F_PARM3,			// Special case for parms
	F_PARM4,			// Special case for parms
	F_PARM5,			// Special case for parms
	F_PARM6,			// Special case for parms
	F_PARM7,			// Special case for parms
	F_PARM8,			// Special case for parms
	F_PARM9,			// Special case for parms
	F_PARM10,			// Special case for parms
	F_PARM11,			// Special case for parms
	F_PARM12,			// Special case for parms
	F_PARM13,			// Special case for parms
	F_PARM14,			// Special case for parms
	F_PARM15,			// Special case for parms
	F_PARM16,			// Special case for parms
	F_FLAG,				// special case for flags
	F_IGNORE
} fieldtype_t;

typedef struct
{
	const char	*name;
	size_t		ofs;
	fieldtype_t	type;
	int		flags;
} field_t;

field_t fields[] = {
	//Fields for benefit of Radiant only
	{"autobound", FOFS(classname), F_IGNORE},
	{"groupname", FOFS(classname), F_IGNORE},
	{"noBasicSounds", FOFS(classname), F_IGNORE},//will be looked at separately
	{"noCombatSounds", FOFS(classname), F_IGNORE},//will be looked at separately
	{"noExtraSounds", FOFS(classname), F_IGNORE},//will be looked at separately

	{"classname", FOFS(classname), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"mins", FOFS(mins), F_VECTOR},
	{"maxs", FOFS(maxs), F_VECTOR},
	{"model", FOFS(model), F_LSTRING},
	{"model2", FOFS(model2), F_LSTRING},
	{"model3", FOFS(target), F_LSTRING},//for misc_replicator_item only!!!
	{"model4", FOFS(target2), F_LSTRING},//for misc_replicator_item only!!!
	{"model5", FOFS(target3), F_LSTRING},//for misc_replicator_item only!!!
	{"model6", FOFS(target4), F_LSTRING},//for misc_replicator_item only!!!
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"duration", FOFS(speed), F_FLOAT},//for psycho jism
	{"interest", FOFS(health), F_INT},//For target_interest
	{"target", FOFS(target), F_LSTRING},
	{"target2", FOFS(target2), F_LSTRING},
	{"target3", FOFS(target3), F_LSTRING},
	{"target4", FOFS(target4), F_LSTRING},
	{"targetJump", FOFS(targetJump), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"material", FOFS(material), F_INT},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"mapname", FOFS(message), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"finaltime", FOFS(wait), F_FLOAT},//For dlight
	{"random", FOFS(random), F_FLOAT},
	{"FOV", FOFS(random), F_FLOAT},//for ref_tags and trigger_visibles
	{"count", FOFS(count), F_INT},
	{"bounceCount", FOFS(bounceCount), F_INT},
	{"health", FOFS(health), F_INT},
	{"friction", FOFS(health), F_INT},//For target_friction_change
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"modelAngles", FOFS(modelAngles), F_VECTOR},
	{"cameraGroup", FOFS(cameraGroup), F_LSTRING},
	{"radius", FOFS(radius), F_FLOAT},
	{"hiderange", FOFS(radius), F_FLOAT},//for triggers only
	{"starttime", FOFS(radius), F_FLOAT},//for dlight
	{"turfrange", FOFS(radius), F_FLOAT},//for sand creatures
	{"type", FOFS(count), F_FLOAT},//for fx_crew_beam_in
	{"fxfile", FOFS(fxFile), F_LSTRING},
	{"fxfile2", FOFS(cameraGroup), F_LSTRING},
	{"noVisTime", FOFS(endFrame), F_INT},//for NPC_Vehicle
	{"endFrame", FOFS(endFrame), F_INT},//for func_usable shader animation
	{"linear", FOFS(alt_fire), F_INT},//for movers to use linear movement
	{"weapon",FOFS(paintarget),F_LSTRING},//for misc_weapon_shooter only

//Script parms - will this handle clamping to 16 or whatever length of parm[0] is?
	{"parm1", 0, F_PARM1},
	{"parm2", 0, F_PARM2},
	{"parm3", 0, F_PARM3},
	{"parm4", 0, F_PARM4},
	{"parm5", 0, F_PARM5},
	{"parm6", 0, F_PARM6},
	{"parm7", 0, F_PARM7},
	{"parm8", 0, F_PARM8},
	{"parm9", 0, F_PARM9},
	{"parm10", 0, F_PARM10},
	{"parm11", 0, F_PARM11},
	{"parm12", 0, F_PARM12},
	{"parm13", 0, F_PARM13},
	{"parm14", 0, F_PARM14},
	{"parm15", 0, F_PARM15},
	{"parm16", 0, F_PARM16},
	//{"noTED", FOFS(s.eFlags), F_FLAG},

//MCG - Begin
	//extra fields for ents
	{"delay", FOFS(delay), F_INT},
	{"sounds", FOFS(sounds), F_INT},
	{"closetarget", FOFS(closetarget), F_LSTRING},//for doors
	{"opentarget", FOFS(opentarget), F_LSTRING},//for doors
	{"paintarget", FOFS(paintarget), F_LSTRING},//for doors
	{"soundGroup", FOFS(paintarget), F_LSTRING},//for target_speakers
	{"backwardstarget", FOFS(paintarget), F_LSTRING},//for trigger_bidirectional
	{"splashDamage", FOFS(splashDamage), F_INT},
	{"splashRadius", FOFS(splashRadius), F_INT},
	//Script stuff
	{"spawnscript", FOFS(behaviorSet[BSET_SPAWN]), F_LSTRING},//name of script to run
	{"usescript", FOFS(behaviorSet[BSET_USE]), F_LSTRING},//name of script to run
	{"awakescript", FOFS(behaviorSet[BSET_AWAKE]), F_LSTRING},//name of script to run
	{"angerscript", FOFS(behaviorSet[BSET_ANGER]), F_LSTRING},//name of script to run
	{"attackscript", FOFS(behaviorSet[BSET_ATTACK]), F_LSTRING},//name of script to run
	{"victoryscript", FOFS(behaviorSet[BSET_VICTORY]), F_LSTRING},//name of script to run
	{"lostenemyscript", FOFS(behaviorSet[BSET_LOSTENEMY]), F_LSTRING},//name of script to run
	{"painscript", FOFS(behaviorSet[BSET_PAIN]), F_LSTRING},//name of script to run
	{"fleescript", FOFS(behaviorSet[BSET_FLEE]), F_LSTRING},//name of script to run
	{"deathscript", FOFS(behaviorSet[BSET_DEATH]), F_LSTRING},//name of script to run
	{"delayscript", FOFS(behaviorSet[BSET_DELAYED]), F_LSTRING},//name of script to run
	{"delayscripttime", FOFS(delayScriptTime), F_INT},//name of script to run
	{"blockedscript", FOFS(behaviorSet[BSET_BLOCKED]), F_LSTRING},//name of script to run
	{"ffirescript", FOFS(behaviorSet[BSET_FFIRE]), F_LSTRING},//name of script to run
	{"ffdeathscript", FOFS(behaviorSet[BSET_FFDEATH]), F_LSTRING},//name of script to run
	{"mindtrickscript", FOFS(behaviorSet[BSET_MINDTRICK]), F_LSTRING},//name of script to run
	{"script_targetname", FOFS(script_targetname), F_LSTRING},//scripts look for this when "affecting"
	//For NPCs
	//{"playerTeam", FOFS(playerTeam), F_INT},
	//{"enemyTeam", FOFS(enemyTeam), F_INT},
	{"NPC_targetname", FOFS(NPC_targetname), F_LSTRING},
	{"NPC_target", FOFS(NPC_target), F_LSTRING},
	{"NPC_target2", FOFS(target2), F_LSTRING},//NPC_spawner only
	{"NPC_target4", FOFS(target4), F_LSTRING},//NPC_spawner only
	{"NPC_type", FOFS(NPC_type), F_LSTRING},
	{"ownername", FOFS(ownername), F_LSTRING},
	//for weapon_saber
	{"saberType", FOFS(NPC_type), F_LSTRING},
	{"saberColor", FOFS(NPC_targetname), F_LSTRING},
	{"saberLeftHand", FOFS(alt_fire), F_INT},
	{"saberSolo", FOFS(loopAnim), F_INT},
	{"saberPitch", FOFS(random), F_FLOAT},
	//freaky camera shit
	{"startRGBA", FOFS(startRGBA), F_VECTOR4},
	{"finalRGBA", FOFS(finalRGBA), F_VECTOR4},
//MCG - End

	{"soundSet", FOFS(soundSet), F_LSTRING},
	{"mass", FOFS(mass), F_FLOAT},		//really only used for pushable misc_model_breakables

//q3map stuff
	{"scale", 0, F_IGNORE},
	{"modelscale", 0, F_IGNORE},
	{"modelscale_vec", 0, F_IGNORE},
	{"style", 0, F_IGNORE},
	{"lip", 0, F_IGNORE},
	{"switch_style", 0, F_IGNORE},
	{"height", 0, F_IGNORE},
	{"noise", 0, F_IGNORE},	//SP_target_speaker
	{"gravity", 0, F_IGNORE},	//SP_target_gravity_change

	{"storyhead", 0, F_IGNORE},		//SP_target_level_change
	{"tier_storyinfo", 0, F_IGNORE},	//SP_target_level_change
	{"zoffset", 0, F_IGNORE},		//used by misc_model_static
	{"music", 0, F_IGNORE},		//used by target_play_music
	{"forcevisible", 0, F_IGNORE},		//for force sight on multiple model entities
	{"redcrosshair", 0, F_IGNORE},		//for red crosshairs on breakables
	{"nodelay", 0, F_IGNORE},		//for Reborn & Cultist NPCs

	{NULL}
};


typedef struct {
	const char	*name;
	void	(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);
void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_breakable (gentity_t *self);
void SP_func_glass( gentity_t *self );
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);
void SP_func_wall (gentity_t *ent);
void SP_func_usable( gentity_t *self );
void SP_rail_mover( gentity_t *self );
void SP_rail_track( gentity_t *self );
void SP_rail_lane( gentity_t *self );



void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_once (gentity_t *ent);
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);
void SP_trigger_bidirectional (gentity_t *ent);
void SP_trigger_entdist (gentity_t *self);
void SP_trigger_location( gentity_t *ent );
void SP_trigger_visible( gentity_t *self );
void SP_trigger_space(gentity_t *self);
void SP_trigger_shipboundary(gentity_t *self);

void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);
void SP_target_speaker (gentity_t *ent);
void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_character (gentity_t *ent);
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_push (gentity_t *ent);
void SP_target_random (gentity_t *self);
void SP_target_counter (gentity_t *self);
void SP_target_scriptrunner (gentity_t *self);
void SP_target_interest (gentity_t *self);
void SP_target_activate (gentity_t *self);
void SP_target_deactivate (gentity_t *self);
void SP_target_gravity_change( gentity_t *self );
void SP_target_friction_change( gentity_t *self );
void SP_target_level_change( gentity_t *self );
void SP_target_change_parm( gentity_t *self );
void SP_target_play_music( gentity_t *self );
void SP_target_autosave( gentity_t *self );
void SP_target_secret( gentity_t *self );

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter (gentity_t *self);
void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_model_static(gentity_t *ent);
void SP_misc_turret (gentity_t *base);
void SP_misc_ns_turret (gentity_t *base);
void SP_laser_arm (gentity_t *base);
void SP_misc_ion_cannon( gentity_t *ent );
void SP_misc_maglock( gentity_t *ent );
void SP_misc_panel_turret( gentity_t *ent );
void SP_misc_model_welder( gentity_t *ent );
void SP_misc_model_jabba_cam( gentity_t *ent );

void SP_misc_model_shield_power_converter( gentity_t *ent );
void SP_misc_model_ammo_power_converter( gentity_t *ent );
void SP_misc_model_bomb_planted( gentity_t *ent );
void SP_misc_model_beacon( gentity_t *ent );

void SP_misc_shield_floor_unit( gentity_t *ent );
void SP_misc_ammo_floor_unit( gentity_t *ent );

void SP_misc_model_gun_rack( gentity_t *ent );
void SP_misc_model_ammo_rack( gentity_t *ent );
void SP_misc_model_cargo_small( gentity_t *ent );

void SP_misc_exploding_crate( gentity_t *ent );
void SP_misc_gas_tank( gentity_t *ent );
void SP_misc_crystal_crate( gentity_t *ent );
void SP_misc_atst_drivable( gentity_t *ent );

void SP_misc_model_breakable(gentity_t *ent);//stays as an ent
void SP_misc_model_ghoul(gentity_t *ent);//stays as an ent
void SP_misc_portal_camera(gentity_t *ent);

void SP_misc_bsp(gentity_t *ent);
void SP_terrain(gentity_t *ent);
void SP_misc_skyportal (gentity_t *ent);

void SP_misc_portal_surface(gentity_t *ent);
void SP_misc_camera_focus (gentity_t *self);
void SP_misc_camera_track (gentity_t *self);
void SP_misc_dlight (gentity_t *ent);
void SP_misc_security_panel (gentity_t *ent);
void SP_misc_camera( gentity_t *ent );
void SP_misc_spotlight( gentity_t *ent );

void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_plasma( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );
void SP_misc_replicator_item( gentity_t *ent );
void SP_misc_trip_mine( gentity_t *self );
void SP_PAS( gentity_t *ent );
void SP_misc_weapon_shooter( gentity_t *self );
void SP_misc_weather_zone( gentity_t *ent );

void SP_misc_cubemap( gentity_t *ent );

//New spawn functions
void SP_reference_tag ( gentity_t *ent );

void SP_NPC_spawner( gentity_t *self );

void SP_NPC_Vehicle( gentity_t *self );
void SP_NPC_Player( gentity_t *self );
void SP_NPC_Kyle( gentity_t *self );
void SP_NPC_Lando( gentity_t *self );
void SP_NPC_Jan( gentity_t *self );
void SP_NPC_Luke( gentity_t *self );
void SP_NPC_MonMothma( gentity_t *self );
void SP_NPC_Rosh_Penin( gentity_t *self );
void SP_NPC_Tavion( gentity_t *self );
void SP_NPC_Tavion_New( gentity_t *self );
void SP_NPC_Alora( gentity_t *self );
void SP_NPC_Reelo( gentity_t *self );
void SP_NPC_Galak( gentity_t *self );
void SP_NPC_Desann( gentity_t *self );
void SP_NPC_Rax( gentity_t *self );
void SP_NPC_BobaFett( gentity_t *self );
void SP_NPC_Ragnos( gentity_t *self );
void SP_NPC_Lannik_Racto( gentity_t *self );
void SP_NPC_Kothos( gentity_t *self );
void SP_NPC_Chewbacca( gentity_t *self );
void SP_NPC_Bartender( gentity_t *self );
void SP_NPC_MorganKatarn( gentity_t *self );
void SP_NPC_Jedi( gentity_t *self );
void SP_NPC_Prisoner( gentity_t *self );
void SP_NPC_Merchant( gentity_t *self );
void SP_NPC_Rebel( gentity_t *self );
void SP_NPC_Human_Merc( gentity_t *self );
void SP_NPC_Stormtrooper( gentity_t *self );
void SP_NPC_StormtrooperOfficer( gentity_t *self );
void SP_NPC_Tie_Pilot( gentity_t *self );
void SP_NPC_Snowtrooper( gentity_t *self );
void SP_NPC_RocketTrooper( gentity_t *self);
void SP_NPC_HazardTrooper( gentity_t *self);
void SP_NPC_Ugnaught( gentity_t *self );
void SP_NPC_Jawa( gentity_t *self );
void SP_NPC_Gran( gentity_t *self );
void SP_NPC_Rodian( gentity_t *self );
void SP_NPC_Weequay( gentity_t *self );
void SP_NPC_Trandoshan( gentity_t *self );
void SP_NPC_Tusken( gentity_t *self );
void SP_NPC_Noghri( gentity_t *self );
void SP_NPC_SwampTrooper( gentity_t *self );
void SP_NPC_Imperial( gentity_t *self );
void SP_NPC_ImpWorker( gentity_t *self );
void SP_NPC_BespinCop( gentity_t *self );
void SP_NPC_Reborn( gentity_t *self );
void SP_NPC_Reborn_New( gentity_t *self);
void SP_NPC_Cultist( gentity_t *self );
void SP_NPC_Cultist_Saber( gentity_t *self );
void SP_NPC_Cultist_Saber_Powers( gentity_t *self );
void SP_NPC_Cultist_Destroyer( gentity_t *self );
void SP_NPC_Cultist_Commando( gentity_t *self );
void SP_NPC_ShadowTrooper( gentity_t *self );
void SP_NPC_Saboteur( gentity_t *self );
void SP_NPC_Monster_Murjj( gentity_t *self );
void SP_NPC_Monster_Swamp( gentity_t *self );
void SP_NPC_Monster_Howler( gentity_t *self );
void SP_NPC_Monster_Rancor( gentity_t *self );
void SP_NPC_Monster_Mutant_Rancor( gentity_t *self );
void SP_NPC_Monster_Wampa( gentity_t *self );
void SP_NPC_Monster_Claw( gentity_t *self );
void SP_NPC_Monster_Glider( gentity_t *self );
void SP_NPC_Monster_Flier2( gentity_t *self );
void SP_NPC_Monster_Lizard( gentity_t *self );
void SP_NPC_Monster_Fish( gentity_t *self );
void SP_NPC_Monster_Sand_Creature( gentity_t *self );
void SP_NPC_MineMonster( gentity_t *self );
void SP_NPC_Droid_Interrogator( gentity_t *self );
void SP_NPC_Droid_Probe( gentity_t *self );
void SP_NPC_Droid_Mark1( gentity_t *self );
void SP_NPC_Droid_Mark2( gentity_t *self );
void SP_NPC_Droid_ATST( gentity_t *self );
void SP_NPC_Droid_Seeker( gentity_t *self );
void SP_NPC_Droid_Remote( gentity_t *self );
void SP_NPC_Droid_Sentry( gentity_t *self );
void SP_NPC_Droid_Gonk( gentity_t *self );
void SP_NPC_Droid_Mouse( gentity_t *self );
void SP_NPC_Droid_R2D2( gentity_t *self );
void SP_NPC_Droid_R5D2( gentity_t *self );
void SP_NPC_Droid_Protocol( gentity_t *self );
void SP_NPC_Droid_Assassin( gentity_t *self);
void SP_NPC_Droid_Saber( gentity_t *self);

void SP_waypoint (gentity_t *ent);
void SP_waypoint_small (gentity_t *ent);
void SP_waypoint_navgoal (gentity_t *ent);

void SP_fx_runner( gentity_t *ent );
void SP_fx_explosion_trail( gentity_t *ent );
void SP_fx_target_beam( gentity_t *ent );
void SP_fx_cloudlayer( gentity_t *ent );

void SP_CreateSnow( gentity_t *ent );
void SP_CreateRain( gentity_t *ent );
void SP_CreateWind( gentity_t *ent );
void SP_CreateWindZone( gentity_t *ent );
// Added 10/20/02 by Aurelio Reis
void SP_CreatePuffSystem( gentity_t *ent );

void SP_object_cargo_barrel1( gentity_t *ent );

void SP_point_combat (gentity_t *self);

void SP_emplaced_eweb( gentity_t *self );
void SP_emplaced_gun( gentity_t *self );

void SP_misc_turbobattery( gentity_t *base );


spawn_t	spawns[] = {
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_breakable", SP_func_breakable},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_timer", SP_func_timer},			// rename trigger_timer?
	{"func_wall", SP_func_wall},
	{"func_usable", SP_func_usable},
	{"func_glass", SP_func_glass},

	{"rail_mover", SP_rail_mover},
	{"rail_track", SP_rail_track},
	{"rail_lane", SP_rail_lane},

	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_once", SP_trigger_once},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_bidirectional", SP_trigger_bidirectional},
	{"trigger_entdist", SP_trigger_entdist},
	{"trigger_location", SP_trigger_location},
	{"trigger_visible", SP_trigger_visible},
	{"trigger_space", SP_trigger_space},
	{"trigger_shipboundary", SP_trigger_shipboundary},

	{"target_give", SP_target_give},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},
	{"target_random", SP_target_random},
	{"target_counter", SP_target_counter},
	{"target_scriptrunner", SP_target_scriptrunner},
	{"target_interest", SP_target_interest},
	{"target_activate", SP_target_activate},
	{"target_deactivate", SP_target_deactivate},
	{"target_gravity_change", SP_target_gravity_change},
	{"target_friction_change", SP_target_friction_change},
	{"target_level_change", SP_target_level_change},
	{"target_change_parm", SP_target_change_parm},
	{"target_play_music", SP_target_play_music},
	{"target_autosave", SP_target_autosave},
	{"target_secret", SP_target_secret},

	{"light", SP_light},
	{"info_null", SP_info_null},
	{"func_group", SP_info_null},
	{"info_notnull", SP_info_notnull},		// use target_position instead
	{"path_corner", SP_path_corner},

	{"misc_teleporter", SP_misc_teleporter},
	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_model_static", SP_misc_model_static},
	{"misc_turret", SP_misc_turret},
	{"misc_ns_turret", SP_misc_ns_turret},
	{"misc_laser_arm", SP_laser_arm},
	{"misc_ion_cannon", SP_misc_ion_cannon},
	{"misc_sentry_turret", SP_PAS},
	{"misc_maglock", SP_misc_maglock},
	{"misc_weapon_shooter", SP_misc_weapon_shooter},
	{"misc_weather_zone", SP_misc_weather_zone},

	{"misc_model_ghoul", SP_misc_model_ghoul},
	{"misc_model_breakable", SP_misc_model_breakable},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"misc_bsp",					SP_misc_bsp},
	{"terrain",						SP_terrain},
	{"misc_skyportal",				SP_misc_skyportal},

	{"misc_camera_focus", SP_misc_camera_focus},
	{"misc_camera_track", SP_misc_camera_track},
	{"misc_dlight", SP_misc_dlight},
	{"misc_replicator_item", SP_misc_replicator_item},
	{"misc_trip_mine", SP_misc_trip_mine},
	{"misc_security_panel", SP_misc_security_panel},
	{"misc_camera", SP_misc_camera},
	{"misc_spotlight", SP_misc_spotlight},
	{"misc_panel_turret", SP_misc_panel_turret},
	{"misc_model_welder", SP_misc_model_welder},
	{"misc_model_jabba_cam", SP_misc_model_jabba_cam},
	{"misc_model_shield_power_converter", SP_misc_model_shield_power_converter},
	{"misc_model_ammo_power_converter", SP_misc_model_ammo_power_converter},
	{"misc_model_bomb_planted", SP_misc_model_bomb_planted},
	{"misc_model_beacon", SP_misc_model_beacon},
	{"misc_shield_floor_unit", SP_misc_shield_floor_unit},
	{"misc_ammo_floor_unit", SP_misc_ammo_floor_unit},

	{"misc_model_gun_rack", SP_misc_model_gun_rack},
	{"misc_model_ammo_rack", SP_misc_model_ammo_rack},
	{"misc_model_cargo_small", SP_misc_model_cargo_small},

	{"misc_exploding_crate", SP_misc_exploding_crate},
	{"misc_gas_tank", SP_misc_gas_tank},
	{"misc_crystal_crate", SP_misc_crystal_crate},
	{"misc_atst_drivable", SP_misc_atst_drivable},

	{"misc_cubemap", SP_misc_cubemap},

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},
	{"shooter_plasma", SP_shooter_plasma},

	{"ref_tag",	SP_reference_tag},

	//new NPC ents
	{"NPC_spawner", SP_NPC_spawner},

	{"NPC_Vehicle", SP_NPC_Vehicle },
	{"NPC_Player", SP_NPC_Player },
	{"NPC_Kyle", SP_NPC_Kyle },
	{"NPC_Lando", SP_NPC_Lando },
	{"NPC_Jan", SP_NPC_Jan },
	{"NPC_Luke", SP_NPC_Luke },
	{"NPC_MonMothma", SP_NPC_MonMothma },
	{"NPC_Rosh_Penin", SP_NPC_Rosh_Penin },
	{"NPC_Tavion", SP_NPC_Tavion },
	{"NPC_Tavion_New", SP_NPC_Tavion_New },
	{"NPC_Alora", SP_NPC_Alora },
	{"NPC_Reelo", SP_NPC_Reelo },
	{"NPC_Galak", SP_NPC_Galak },
	{"NPC_Desann", SP_NPC_Desann },
	{"NPC_Rax", SP_NPC_Rax },
	{"NPC_BobaFett", SP_NPC_BobaFett },
	{"NPC_Ragnos", SP_NPC_Ragnos },
	{"NPC_Lannik_Racto", SP_NPC_Lannik_Racto },
	{"NPC_Kothos", SP_NPC_Kothos },
	{"NPC_Chewbacca", SP_NPC_Chewbacca },
	{"NPC_Bartender", SP_NPC_Bartender },
	{"NPC_MorganKatarn", SP_NPC_MorganKatarn },
	{"NPC_Jedi", SP_NPC_Jedi },
	{"NPC_Prisoner", SP_NPC_Prisoner },
	{"NPC_Merchant", SP_NPC_Merchant },
	{"NPC_Rebel", SP_NPC_Rebel },
	{"NPC_Human_Merc", SP_NPC_Human_Merc },
	{"NPC_Stormtrooper", SP_NPC_Stormtrooper },
	{"NPC_StormtrooperOfficer", SP_NPC_StormtrooperOfficer },
	{"NPC_Tie_Pilot", SP_NPC_Tie_Pilot },
	{"NPC_Snowtrooper", SP_NPC_Snowtrooper },
	{"NPC_RocketTrooper", SP_NPC_RocketTrooper },
	{"NPC_HazardTrooper", SP_NPC_HazardTrooper },

	{"NPC_Ugnaught", SP_NPC_Ugnaught },
	{"NPC_Jawa", SP_NPC_Jawa },
	{"NPC_Gran", SP_NPC_Gran },
	{"NPC_Rodian", SP_NPC_Rodian },
	{"NPC_Weequay", SP_NPC_Weequay },
	{"NPC_Trandoshan", SP_NPC_Trandoshan },
	{"NPC_Tusken", SP_NPC_Tusken },
	{"NPC_Noghri", SP_NPC_Noghri },
	{"NPC_SwampTrooper", SP_NPC_SwampTrooper },
	{"NPC_Imperial", SP_NPC_Imperial },
	{"NPC_ImpWorker", SP_NPC_ImpWorker },
	{"NPC_BespinCop", SP_NPC_BespinCop },
	{"NPC_Reborn", SP_NPC_Reborn },
	{"NPC_Reborn_New", SP_NPC_Reborn_New },
	{"NPC_Cultist", SP_NPC_Cultist },
	{"NPC_Cultist_Saber", SP_NPC_Cultist_Saber },
	{"NPC_Cultist_Saber_Powers", SP_NPC_Cultist_Saber_Powers },
	{"NPC_Cultist_Destroyer", SP_NPC_Cultist_Destroyer },
	{"NPC_Cultist_Commando", SP_NPC_Cultist_Commando },
	{"NPC_ShadowTrooper", SP_NPC_ShadowTrooper },
	{"NPC_Saboteur", SP_NPC_Saboteur },
	{"NPC_Monster_Murjj", SP_NPC_Monster_Murjj },
	{"NPC_Monster_Swamp", SP_NPC_Monster_Swamp },
	{"NPC_Monster_Howler", SP_NPC_Monster_Howler },
	{"NPC_Monster_Rancor", SP_NPC_Monster_Rancor },
	{"NPC_Monster_Mutant_Rancor", SP_NPC_Monster_Mutant_Rancor },
	{"NPC_Monster_Wampa", SP_NPC_Monster_Wampa },
	{"NPC_MineMonster",	SP_NPC_MineMonster },
	{"NPC_Monster_Claw", SP_NPC_Monster_Claw },
	{"NPC_Monster_Glider", SP_NPC_Monster_Glider },
	{"NPC_Monster_Flier2", SP_NPC_Monster_Flier2 },
	{"NPC_Monster_Lizard", SP_NPC_Monster_Lizard },
	{"NPC_Monster_Fish", SP_NPC_Monster_Fish },
	{"NPC_Monster_Sand_Creature", SP_NPC_Monster_Sand_Creature },
	{"NPC_Droid_Interrogator", SP_NPC_Droid_Interrogator },
	{"NPC_Droid_Probe", SP_NPC_Droid_Probe },
	{"NPC_Droid_Mark1", SP_NPC_Droid_Mark1 },
	{"NPC_Droid_Mark2", SP_NPC_Droid_Mark2 },
	{"NPC_Droid_ATST", SP_NPC_Droid_ATST },
	{"NPC_Droid_Seeker", SP_NPC_Droid_Seeker },
	{"NPC_Droid_Remote", SP_NPC_Droid_Remote },
	{"NPC_Droid_Sentry", SP_NPC_Droid_Sentry },
	{"NPC_Droid_Gonk", SP_NPC_Droid_Gonk },
	{"NPC_Droid_Mouse", SP_NPC_Droid_Mouse },
	{"NPC_Droid_R2D2", SP_NPC_Droid_R2D2 },
	{"NPC_Droid_R5D2", SP_NPC_Droid_R5D2 },
	{"NPC_Droid_Protocol", SP_NPC_Droid_Protocol },
	{"NPC_Droid_Assassin", SP_NPC_Droid_Assassin },
	{"NPC_Droid_Saber", SP_NPC_Droid_Saber },

	//rwwFIXMEFIXME: Faked for testing NPCs (another other things) in RMG with sof2 assets
	{"NPC_Colombian_Soldier", SP_NPC_Reborn },
	{"NPC_Colombian_Rebel", SP_NPC_Reborn },
	{"NPC_Colombian_EmplacedGunner", SP_NPC_ShadowTrooper },
	{"NPC_Manuel_Vergara_RMG", SP_NPC_Desann },
//	{"info_NPCnav", SP_waypoint},

	{"waypoint", SP_waypoint},
	{"waypoint_small", SP_waypoint_small},
	{"waypoint_navgoal", SP_waypoint_navgoal},

	{"fx_runner", SP_fx_runner},
	{"fx_explosion_trail", SP_fx_explosion_trail},
	{"fx_target_beam", SP_fx_target_beam},
	{"fx_cloudlayer", SP_fx_cloudlayer},
	{"fx_rain", SP_CreateRain},
	{"fx_wind", SP_CreateWind},
	{"fx_snow", SP_CreateSnow},
	{"fx_puff", SP_CreatePuffSystem},
	{"fx_wind_zone", SP_CreateWindZone},

	{"object_cargo_barrel1", SP_object_cargo_barrel1},
	{"point_combat", SP_point_combat},

	{"emplaced_gun", SP_emplaced_gun},
	{"emplaced_eweb", SP_emplaced_eweb},

	{NULL, NULL}
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t	*s;
	gitem_t	*item;

	if ( !ent->classname ) {
		gi.Printf (S_COLOR_RED"G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, ent->classname) ) {
			// found it
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, ent->classname) ) {
			// found it
			s->spawn(ent);
			return qtrue;
		}
	}
	char* str;
	G_SpawnString( "origin", "?", &str );
	gi.Printf (S_COLOR_RED"ERROR: %s is not a spawn function @(%s)\n", ent->classname, str);
	delayedShutDown = level.time + 100;
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	int		i,l;

	if(!string || !string[0])
	{
		//gi.Printf(S_COLOR_RED"Error: G_NewString called with NULL string!\n");
		return NULL;
	}

	l = strlen(string) + 1;

	newb = (char *) G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void Q3_SetParm (int entID, int parmNum, const char *parmValue);
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;
	vec4_t	vec4;

	for ( f=fields ; f->name ; f++ ) {
		if ( !Q_stricmp(f->name, key) ) {
			// found it
			b = (byte *)ent;

			switch( f->type ) {
			case F_LSTRING:
				*(char **)(b+f->ofs) = G_NewString (value);
				break;
			case F_VECTOR:
			{
				int _iFieldsRead = sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				assert(_iFieldsRead==3);
				if (_iFieldsRead!=3)
				{
					gi.Printf (S_COLOR_YELLOW"G_ParseField: VEC3 sscanf() failed to read 3 floats ('angle' key bug?)\n");
					delayedShutDown = level.time + 100;
				}
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			}
			case F_VECTOR4:
			{
				int _iFieldsRead =  sscanf (value, "%f %f %f %f", &vec4[0], &vec4[1], &vec4[2], &vec4[3]);
				assert(_iFieldsRead==4);
				if (_iFieldsRead!=4)
				{
					gi.Printf (S_COLOR_YELLOW"G_ParseField: VEC4 sscanf() failed to read 4 floats\n");
					delayedShutDown = level.time + 100;
				}
				((float *)(b+f->ofs))[0] = vec4[0];
				((float *)(b+f->ofs))[1] = vec4[1];
				((float *)(b+f->ofs))[2] = vec4[2];
				((float *)(b+f->ofs))[3] = vec4[3];
				break;
			}
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			case F_PARM1:
			case F_PARM2:
			case F_PARM3:
			case F_PARM4:
			case F_PARM5:
			case F_PARM6:
			case F_PARM7:
			case F_PARM8:
			case F_PARM9:
			case F_PARM10:
			case F_PARM11:
			case F_PARM12:
			case F_PARM13:
			case F_PARM14:
			case F_PARM15:
			case F_PARM16:
				Q3_SetParm( ent->s.number, (f->type - F_PARM1), (char *) value );
				break;
			case F_FLAG:
				{//try to find the proper flag for that key:
					int flag = GetIDForString ( flagTable, key );

					if ( flag > 0 )
					{
						G_SpawnFlag( key, flag, (int *)(b+f->ofs) );
					}
					else
					{
#ifndef FINAL_BUILD
						gi.Printf (S_COLOR_YELLOW"WARNING: G_ParseField: can't find flag for key %s\n", key);
#endif
					}
				}
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
#ifndef FINAL_BUILD
	//didn't find it?
	if (key[0]!='_')
	{
		gi.Printf ( S_COLOR_YELLOW"WARNING: G_ParseField: no such field: %s\n", key );
	}
#endif
}

static qboolean SpawnForCurrentDifficultySetting( gentity_t *ent )
{
extern cvar_t	*com_buildScript;
	if (com_buildScript->integer) {	//always spawn when building a pak file
		return qtrue;
	}
	if ( ent->spawnflags & ( 1 << (8 + g_spskill->integer )) )	{// easy -256	medium -512		hard -1024
		return qfalse;
	} else {
		return qtrue;
	}
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/

void G_SpawnGEntityFromSpawnVars( void ) {
	int			i;
	gentity_t	*ent;

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < numSpawnVars ; i++ ) {
		G_ParseField( spawnVars[i][0], spawnVars[i][1], ent );
	}

	G_SpawnInt( "notsingle", "0", &i );
	if ( i || !SpawnForCurrentDifficultySetting( ent ) ) {
		G_FreeEntity( ent );
		return;
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
		return;
	}

	//Tag on the ICARUS scripting information only to valid recipients
	if ( Quake3Game()->ValidEntity( ent ) )
	{
		Quake3Game()->InitEntity( ent ); //ICARUS_InitEnt( ent );

		if ( ent->classname && ent->classname[0] )
		{
			if ( Q_strncmp( "NPC_", ent->classname, 4 ) != 0 )
			{//Not an NPC_spawner
				G_ActivateBehavior( ent, BSET_SPAWN );
			}
		}
	}
}

void G_SpawnSubBSPGEntityFromSpawnVars( vec3_t posOffset, vec3_t angOffset ) {
	int			i;
	gentity_t	*ent;

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < numSpawnVars ; i++ ) {
		G_ParseField( spawnVars[i][0], spawnVars[i][1], ent );
	}

	G_SpawnInt( "notsingle", "0", &i );
	if ( i || !SpawnForCurrentDifficultySetting( ent ) ) {
		G_FreeEntity( ent );
		return;
	}

	VectorAdd(ent->s.origin, posOffset, ent->s.origin);
	VectorAdd(ent->s.angles, angOffset, ent->s.angles);

	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorCopy(ent->s.angles, ent->currentAngles);

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
		return;
	}

	//Tag on the ICARUS scripting information only to valid recipients
	if ( Quake3Game()->ValidEntity( ent ) )
	{

		Quake3Game()->InitEntity( ent ); // ICARUS_InitEnt( ent );

		if ( ent->classname && ent->classname[0] )
		{
			if ( Q_strncmp( "NPC_", ent->classname, 4 ) != 0 )
			{//Not an NPC_spawner
				G_ActivateBehavior( ent, BSET_SPAWN );
			}
		}
	}
}


/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = spawnVarChars + numSpawnVarChars;
	memcpy( dest, string, l+1 );

	numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( const char **data ) {
	char		keyname[MAX_STRING_CHARS];
	const char	*com_token;

	numSpawnVars = 0;
	numSpawnVarChars = 0;

	// parse the opening brace
	COM_BeginParseSession();
	com_token = COM_Parse( data );
	if ( !*data ) {
		// end of spawn string
		COM_EndParseSession();
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		COM_EndParseSession();
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		com_token = COM_Parse( data );
		if ( !*data ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			break;
		}

		Q_strncpyz( keyname, com_token, sizeof(keyname) );

		// parse value
		com_token = COM_Parse( data );
		if ( !*data ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}
		if ( com_token[0] == '}' ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( numSpawnVars == MAX_SPAWN_VARS ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		spawnVars[ numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		spawnVars[ numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		numSpawnVars++;
	}

	COM_EndParseSession();
	return qtrue;
}

static	const char *defaultStyles[LS_NUM_STYLES][3] =
{
	{	// 0 normal
		"z",
		"z",
		"z"
	},
	{	// 1 FLICKER (first variety)
		"mmnmmommommnonmmonqnmmo",
		"mmnmmommommnonmmonqnmmo",
		"mmnmmommommnonmmonqnmmo"
	},
	{	// 2 SLOW STRONG PULSE
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb",
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb",
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb"
	},
	{	// 3 CANDLE (first variety)
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
	},
	{	// 4 FAST STROBE
		"mamamamamama",
		"mamamamamama",
		"mamamamamama"
	},
	{	// 5 GENTLE PULSE 1
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj"
	},
	{	// 6 FLICKER (second variety)
		"nmonqnmomnmomomno",
		"nmonqnmomnmomomno",
		"nmonqnmomnmomomno"
	},
	{	// 7 CANDLE (second variety)
		"mmmaaaabcdefgmmmmaaaammmaamm",
		"mmmaaaabcdefgmmmmaaaammmaamm",
		"mmmaaaabcdefgmmmmaaaammmaamm"
	},
	{	// 8 CANDLE (third variety)
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa"
	},
	{	// 9 SLOW STROBE (fourth variety)
		"aaaaaaaazzzzzzzz",
		"aaaaaaaazzzzzzzz",
		"aaaaaaaazzzzzzzz"
	},
	{	// 10 FLUORESCENT FLICKER
		"mmamammmmammamamaaamammma",
		"mmamammmmammamamaaamammma",
		"mmamammmmammamamaaamammma"
	},
	{	// 11 SLOW PULSE NOT FADE TO BLACK
		"abcdefghijklmnopqrrqponmlkjihgfedcba",
		"abcdefghijklmnopqrrqponmlkjihgfedcba",
		"abcdefghijklmnopqrrqponmlkjihgfedcba"
	},
	{	// 12 FAST PULSE FOR JEREMY
		"mkigegik",
		"mkigegik",
		"mkigegik"
	},
	{	// 13 Test Blending
		"abcdefghijklmqrstuvwxyz",
		"zyxwvutsrqmlkjihgfedcba",
		"aammbbzzccllcckkffyyggp"
	},
	{	// 14
		"",
		"",
		""
	},
	{	// 15
		"",
		"",
		""
	},
	{	// 16
		"",
		"",
		""
	},
	{	// 17
		"",
		"",
		""
	},
	{	// 18
		"",
		"",
		""
	},
	{	// 19
		"",
		"",
		""
	},
	{	// 20
		"",
		"",
		""
	},
	{	// 21
		"",
		"",
		""
	},
	{	// 22
		"",
		"",
		""
	},
	{	// 23
		"",
		"",
		""
	},
	{	// 24
		"",
		"",
		""
	},
	{	// 25
		"",
		"",
		""
	},
	{	// 26
		"",
		"",
		""
	},
	{	// 27
		"",
		"",
		""
	},
	{	// 28
		"",
		"",
		""
	},
	{	// 29
		"",
		"",
		""
	},
	{	// 30
		"",
		"",
		""
	},
	{	// 31
		"",
		"",
		""
	}
};


/*QUAKED worldspawn (0 0 0) ?
Every map should have exactly one worldspawn.
"music"     path to WAV or MP3 files (e.g. "music\intro.mp3 music\loopfile.mp3")
"gravity"   800 is default gravity
"message"   Text to print during connection
"soundSet"  Ambient sound set to play
"spawnscript" runs this script

BSP Options
"gridsize"     size of lighting grid to "X Y Z". default="64 64 128"
"ambient"      amount of global light to add to each surf (uses _color)
"chopsize"     value for bsp on the maximum polygon / portal size
"distancecull" value for vis for the maximum viewing distance
"_minlight"   minimum lighting on a surf.  overrides _mingridlight and _minvertexlight

Game Options
"fog"          shader name of the global fog texture - must include the full path, such as "textures/rj/fog1"
"ls_Xr"	override lightstyle X with this pattern for Red.
"ls_Xg"	green (valid patterns are "a-z")
"ls_Xb"	blue (a is OFF, z is ON)
"breath"		Whether the entity's have breath puffs or not (0 = No, 1 = All, 2 = Just cold breath, 3 = Just under water bubbles ).
"clearstats" default 1, if 0 loading this map will not clear the stats for player
"tier_storyinfo" sets 'tier_storyinfo' cvar
*/
void SP_worldspawn( void ) {
	char	*s;
	int		i;

	g_entities[ENTITYNUM_WORLD].max_health = 0;

	for ( i = 0 ; i < numSpawnVars ; i++ )
	{
		if ( Q_stricmp( "spawnscript", spawnVars[i][0] ) == 0 )
		{//ONly let them set spawnscript, we don't want them setting an angle or something on the world.
			G_ParseField( spawnVars[i][0], spawnVars[i][1], &g_entities[ENTITYNUM_WORLD] );
		}
		if ( Q_stricmp( "region", spawnVars[i][0] ) == 0 )
		{
			g_entities[ENTITYNUM_WORLD].s.radius = atoi(spawnVars[i][1]);
		}
		if ( Q_stricmp( "distancecull", spawnVars[i][0] ) == 0 )
		{
			g_entities[ENTITYNUM_WORLD].max_health = (int)((float)(atoi(spawnVars[i][1])) * 0.7f);
		}
	}

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	G_SpawnString( "music", "", &s );
	gi.SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	gi.SetConfigstring( CS_MESSAGE, s );				// map specific message

	G_SpawnString( "gravity", "800", &s );
	extern SavedGameJustLoaded_e g_eSavedGameJustLoaded;
	if (g_eSavedGameJustLoaded != eFULL)
	{
		gi.cvar_set( "g_gravity", s );
	}

	G_SpawnString( "soundSet", "default", &s );
	gi.SetConfigstring( CS_AMBIENT_SET, s );

	//Lightstyles
	gi.SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+0, defaultStyles[0][0]);
	gi.SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+1, defaultStyles[0][1]);
	gi.SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+2, defaultStyles[0][2]);

	for(i=1;i<LS_NUM_STYLES;i++)
	{
		char	temp[32];
		int		lengthRed, lengthBlue, lengthGreen;
		Com_sprintf(temp, sizeof(temp), "ls_%dr", i);
		G_SpawnString( temp, defaultStyles[i][0], &s );
		lengthRed = strlen(s);
		gi.SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+0, s);

		Com_sprintf(temp, sizeof(temp), "ls_%dg", i);
		G_SpawnString(temp, defaultStyles[i][1], &s);
		lengthGreen = strlen(s);
		gi.SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+1, s);

		Com_sprintf(temp, sizeof(temp), "ls_%db", i);
		G_SpawnString(temp, defaultStyles[i][2], &s);
		lengthBlue = strlen(s);
		gi.SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+2, s);

		if (lengthRed != lengthGreen || lengthGreen != lengthBlue)
		{
			Com_Error(ERR_DROP, "Style %d has inconsistent lengths: R %d, G %d, B %d",
				i, lengthRed, lengthGreen, lengthBlue);
		}
	}

	G_SpawnString( "breath", "0", &s );
	gi.cvar_set( "cg_drawBreath", s );

	G_SpawnString( "clearstats", "1", &s );
	gi.cvar_set( "g_clearstats", s );

	if (G_SpawnString( "tier_storyinfo", "", &s ))
	{
		gi.cvar_set( "tier_storyinfo", s );
	}

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";
}

/*
-------------------------
G_ParsePrecaches
-------------------------
*/

void G_ParsePrecaches( void )
{
	gentity_t	*ent = NULL;

	//Clear any old lists
	if(!as_preCacheMap) {
		as_preCacheMap = new namePrecache_m;
	}

	as_preCacheMap->clear();

	for ( int i = 0; i < globals.num_entities; i++ )
	{
		ent = &g_entities[i];

		if VALIDSTRING( ent->soundSet )
		{
			(*as_preCacheMap)[ (char *) ent->soundSet ] = 1;
		}
	}
}


void G_ASPreCacheFree(void)
{
	if(as_preCacheMap) {
		delete as_preCacheMap;
		as_preCacheMap = NULL;
	}
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
extern int num_waypoints;
extern void	RG_RouteGen(void);
extern qboolean NPCsPrecached;

qboolean SP_bsp_worldspawn ( void )
{
	return qtrue;
}

void G_SubBSPSpawnEntitiesFromString(const char *entityString, vec3_t posOffset, vec3_t angOffset)
{
	const char		*entities;

	entities = entityString;

	// allow calls to G_Spawn*()
	spawning = qtrue;
	NPCsPrecached = qfalse;
	numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars( &entities ) ) {
		G_Error( "SpawnEntities: no entities" );
	}

	// Skip this guy if its worldspawn fails
	if ( !SP_bsp_worldspawn() )
	{
		return;
	}

	// parse ents
	while( G_ParseSpawnVars(&entities) )
	{
		G_SpawnSubBSPGEntityFromSpawnVars(posOffset, angOffset);
	}
}

void G_SpawnEntitiesFromString( const char *entityString ) {
	const char		*entities;

	entities = entityString;

	// allow calls to G_Spawn*()
	spawning = qtrue;
	NPCsPrecached = qfalse;
	numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars( &entities ) ) {
		G_Error( "SpawnEntities: no entities" );
	}

	SP_worldspawn();

	// parse ents
	while( G_ParseSpawnVars( &entities ) )
	{
		G_SpawnGEntityFromSpawnVars();
	}

	//Search the entities for precache information
	G_ParsePrecaches();


	if( g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN] && g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN][0] )
	{//World has a spawn script, but we don't want the world in ICARUS and running scripts,
		//so make a scriptrunner and start it going.
		gentity_t *script_runner = G_Spawn();
		if ( script_runner )
		{
			script_runner->behaviorSet[BSET_USE] = g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN];
			script_runner->count = 1;
			script_runner->e_ThinkFunc = thinkF_scriptrunner_run;
			script_runner->nextthink = level.time + 100;

			if ( Quake3Game()->ValidEntity( script_runner ) )
			{
				Quake3Game()->InitEntity( script_runner ); //ICARUS_InitEnt( script_runner );
			}
		}
	}

	//gi.Printf(S_COLOR_YELLOW"Total waypoints: %d\n", num_waypoints);
	//Automatically run routegen
	//RG_RouteGen();

	spawning = qfalse;			// any future calls to G_Spawn*() will be errors

	if ( g_delayedShutdown->integer && delayedShutDown )
	{
		assert(0);
		G_Error( "Errors loading map, check the console for them." );
	}
}

