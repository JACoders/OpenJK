// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
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



BG_field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"teamnodmg", FOFS(teamnodmg), F_INT},
	{"teamowner", FOFS(s.teamowner), F_INT},
	{"teamuser", FOFS(alliedTeam), F_INT},
	{"alliedTeam", FOFS(alliedTeam), F_INT},//for misc_turrets
	{"roffname", FOFS(roffname), F_LSTRING},
	{"rofftarget", FOFS(rofftarget), F_LSTRING},
	{"healingclass", FOFS(healingclass), F_LSTRING},
	{"healingsound", FOFS(healingsound), F_LSTRING},
	{"healingrate", FOFS(healingrate), F_INT},
	{"ownername", FOFS(ownername), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_LSTRING},
	{"model2", FOFS(model2), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"target2", FOFS(target2), F_LSTRING},
	{"target3", FOFS(target3), F_LSTRING},
	{"target4", FOFS(target4), F_LSTRING},
	{"target5", FOFS(target5), F_LSTRING},
	{"target6", FOFS(target6), F_LSTRING},
	{"NPC_targetname", FOFS(NPC_targetname), F_LSTRING},
	{"NPC_target", FOFS(NPC_target), F_LSTRING},
	{"NPC_target2", FOFS(target2), F_LSTRING},//NPC_spawner only
	{"NPC_target4", FOFS(target4), F_LSTRING},//NPC_spawner only
	{"NPC_type", FOFS(NPC_type), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_INT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"targetShaderName", FOFS(targetShaderName), F_LSTRING},
	{"targetShaderNewName", FOFS(targetShaderNewName), F_LSTRING},
	{"linear", FOFS(alt_fire), F_INT},//for movers to use linear movement

	{"closetarget", FOFS(closetarget), F_LSTRING},//for doors
	{"opentarget", FOFS(opentarget), F_LSTRING},//for doors
	{"paintarget", FOFS(paintarget), F_LSTRING},//for doors

	{"goaltarget", FOFS(goaltarget), F_LSTRING},//for siege
	{"idealclass", FOFS(idealclass), F_LSTRING},//for siege spawnpoints

	//rww - icarus stuff:
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

	{"fullName", FOFS(fullName), F_LSTRING},

	{"soundSet", FOFS(soundSet), F_LSTRING},
	{"radius", FOFS(radius), F_FLOAT},
	{"numchunks", FOFS(radius), F_FLOAT},//for func_breakables
	{"chunksize", FOFS(mass), F_FLOAT},//for func_breakables

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

	{NULL}
};


typedef struct {
	char	*name;
	void	(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_duel( gentity_t *ent );
void SP_info_player_duel1( gentity_t *ent );
void SP_info_player_duel2( gentity_t *ent );
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_siegeteam1 (gentity_t *ent);
void SP_info_player_siegeteam2 (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);
void SP_info_player_intermission_red (gentity_t *ent);
void SP_info_player_intermission_blue (gentity_t *ent);
void SP_info_jedimaster_start (gentity_t *ent);
void SP_info_player_start_red (gentity_t *ent);
void SP_info_player_start_blue (gentity_t *ent);
void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);
void SP_info_podium(gentity_t *ent);

void SP_info_siege_objective (gentity_t *ent);
void SP_info_siege_radaricon (gentity_t *ent);
void SP_info_siege_decomplete (gentity_t *ent);
void SP_target_siege_end (gentity_t *ent);
void SP_misc_siege_item (gentity_t *ent);

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);
void SP_func_breakable (gentity_t *ent);
void SP_func_glass (gentity_t *ent);
void SP_func_usable( gentity_t *ent);
void SP_func_wall( gentity_t *ent );

void SP_trigger_lightningstrike( gentity_t *ent );

void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_once( gentity_t *ent );
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);
void SP_trigger_space(gentity_t *self);
void SP_trigger_shipboundary(gentity_t *self);
void SP_trigger_hyperspace(gentity_t *self);
void SP_trigger_asteroid_field(gentity_t *self);

void SP_target_remove_powerups( gentity_t *ent );
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
void SP_target_counter (gentity_t *self);
void SP_target_random (gentity_t *self);
void SP_target_scriptrunner( gentity_t *self );
void SP_target_interest (gentity_t *self);
void SP_target_activate (gentity_t *self);
void SP_target_deactivate (gentity_t *self);
void SP_target_level_change( gentity_t *self );
void SP_target_play_music( gentity_t *self );
void SP_target_push (gentity_t *ent);

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_model_static(gentity_t *ent);
void SP_misc_G2model(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);
void SP_misc_weather_zone( gentity_t *ent );

void SP_misc_bsp (gentity_t *ent);
void SP_terrain (gentity_t *ent);
void SP_misc_skyportal_orient (gentity_t *ent);
void SP_misc_skyportal (gentity_t *ent);

void SP_misc_ammo_floor_unit(gentity_t *ent);
void SP_misc_shield_floor_unit( gentity_t *ent );
void SP_misc_model_shield_power_converter( gentity_t *ent );
void SP_misc_model_ammo_power_converter( gentity_t *ent );
void SP_misc_model_health_power_converter( gentity_t *ent );

void SP_fx_runner( gentity_t *ent );

void SP_target_screenshake(gentity_t *ent);
void SP_target_escapetrig(gentity_t *ent);

void SP_misc_maglock ( gentity_t *self );

void SP_misc_faller(gentity_t *ent);

void SP_misc_holocron(gentity_t *ent);

void SP_reference_tag ( gentity_t *ent );

void SP_misc_weapon_shooter( gentity_t *self );

void SP_NPC_spawner( gentity_t *self );

void SP_NPC_Vehicle( gentity_t *self);

void SP_NPC_Kyle( gentity_t *self );
void SP_NPC_Lando( gentity_t *self );
void SP_NPC_Jan( gentity_t *self );
void SP_NPC_Luke( gentity_t *self );
void SP_NPC_MonMothma( gentity_t *self );
void SP_NPC_Tavion( gentity_t *self );
void SP_NPC_Tavion_New( gentity_t *self );
void SP_NPC_Alora( gentity_t *self );
void SP_NPC_Reelo( gentity_t *self );
void SP_NPC_Galak( gentity_t *self );
void SP_NPC_Desann( gentity_t *self );
void SP_NPC_Bartender( gentity_t *self );
void SP_NPC_MorganKatarn( gentity_t *self );
void SP_NPC_Jedi( gentity_t *self );
void SP_NPC_Prisoner( gentity_t *self );
void SP_NPC_Rebel( gentity_t *self );
void SP_NPC_Stormtrooper( gentity_t *self );
void SP_NPC_StormtrooperOfficer( gentity_t *self );
void SP_NPC_Snowtrooper( gentity_t *self);
void SP_NPC_Tie_Pilot( gentity_t *self );
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
void SP_NPC_ShadowTrooper( gentity_t *self );
void SP_NPC_Monster_Murjj( gentity_t *self );
void SP_NPC_Monster_Swamp( gentity_t *self );
void SP_NPC_Monster_Howler( gentity_t *self );
void SP_NPC_Monster_Claw( gentity_t *self );
void SP_NPC_Monster_Glider( gentity_t *self );
void SP_NPC_Monster_Flier2( gentity_t *self );
void SP_NPC_Monster_Lizard( gentity_t *self );
void SP_NPC_Monster_Fish( gentity_t *self );
void SP_NPC_Monster_Wampa( gentity_t *self );
void SP_NPC_Monster_Rancor( gentity_t *self );
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

void SP_NPC_Reborn_New( gentity_t *self);
void SP_NPC_Cultist( gentity_t *self );
void SP_NPC_Cultist_Saber( gentity_t *self );
void SP_NPC_Cultist_Saber_Powers( gentity_t *self );
void SP_NPC_Cultist_Destroyer( gentity_t *self );
void SP_NPC_Cultist_Commando( gentity_t *self );

void SP_waypoint (gentity_t *ent);
void SP_waypoint_small (gentity_t *ent);
void SP_waypoint_navgoal (gentity_t *ent);
void SP_waypoint_navgoal_8 (gentity_t *ent);
void SP_waypoint_navgoal_4 (gentity_t *ent);
void SP_waypoint_navgoal_2 (gentity_t *ent);
void SP_waypoint_navgoal_1 (gentity_t *ent);

void SP_CreateSpaceDust( gentity_t *ent );
void SP_CreateSnow( gentity_t *ent );
void SP_CreateRain( gentity_t *ent );

void SP_point_combat( gentity_t *self );

void SP_shooter_blaster( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

void SP_misc_turret( gentity_t *ent );
void SP_misc_turretG2( gentity_t *base );


void SP_item_botroam( gentity_t *ent )
{
}

void SP_gametype_item ( gentity_t* ent )
{
	gitem_t *item = NULL;
	char *value;
	int team = -1;

	G_SpawnString("teamfilter", "", &value);

	G_SetOrigin( ent, ent->s.origin );

	// If a team filter is set then override any team settings for the spawns
	if ( level.mTeamFilter[0] )
	{
		if ( Q_stricmp ( level.mTeamFilter, "red") == 0 )
		{
			team = TEAM_RED;
		}
		else if ( Q_stricmp ( level.mTeamFilter, "blue") == 0 )
		{
			team = TEAM_BLUE;
		}
	}

	if (ent->targetname && ent->targetname[0])
	{
		if (team != -1)
		{
			if (strstr(ent->targetname, "flag"))
			{
				if (team == TEAM_RED)
				{
					item = BG_FindItem("team_CTF_redflag");
				}
				else
				{ //blue
					item = BG_FindItem("team_CTF_blueflag");
				}
			}
		}
		else if (strstr(ent->targetname, "red_flag"))
		{
			item = BG_FindItem("team_CTF_redflag");
		}
		else if (strstr(ent->targetname, "blue_flag"))
		{
			item = BG_FindItem("team_CTF_blueflag");
		}
		else
		{
			item = NULL;
		}

		if (item)
		{
			ent->targetname = NULL;
			ent->classname = item->classname;
			G_SpawnItem( ent, item );
		}
	}
}

void SP_emplaced_gun( gentity_t *ent );

spawn_t	spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_duel", SP_info_player_duel},
	{"info_player_duel1", SP_info_player_duel1},
	{"info_player_duel2", SP_info_player_duel2},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_siegeteam1", SP_info_player_siegeteam1},
	{"info_player_siegeteam2", SP_info_player_siegeteam2},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_player_intermission_red", SP_info_player_intermission_red},
	{"info_player_intermission_blue", SP_info_player_intermission_blue},
	{"info_jedimaster_start", SP_info_jedimaster_start},
	{"info_player_start_red", SP_info_player_start_red},
	{"info_player_start_blue", SP_info_player_start_blue},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},		// use target_position instead
	{"info_camp", SP_info_camp},

	{"info_siege_objective", SP_info_siege_objective},
	{"info_siege_radaricon", SP_info_siege_radaricon},
	{"info_siege_decomplete", SP_info_siege_decomplete},
	{"target_siege_end", SP_target_siege_end},
	{"misc_siege_item", SP_misc_siege_item},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	{"func_timer", SP_func_timer},			// rename trigger_timer?
	{"func_breakable", SP_func_breakable},
	{"func_glass", SP_func_glass},
	{"func_usable", SP_func_usable},
	{"func_wall", SP_func_wall},

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_lightningstrike", SP_trigger_lightningstrike},

	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_once", SP_trigger_once},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_space", SP_trigger_space},
	{"trigger_shipboundary", SP_trigger_shipboundary},
	{"trigger_hyperspace", SP_trigger_hyperspace},
	{"trigger_asteroid_field", SP_trigger_asteroid_field},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
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
	{"target_counter", SP_target_counter},
	{"target_random", SP_target_random},
	{"target_scriptrunner", SP_target_scriptrunner},
	{"target_interest", SP_target_interest},
	{"target_activate", SP_target_activate},
	{"target_deactivate", SP_target_deactivate},
	{"target_level_change", SP_target_level_change},
	{"target_play_music", SP_target_play_music},
	{"target_push", SP_target_push},

	{"light", SP_light},
	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_model_static", SP_misc_model_static},
	{"misc_G2model", SP_misc_G2model},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},
	{"misc_weather_zone", SP_misc_weather_zone},

	{"misc_bsp", SP_misc_bsp},
	{"terrain", SP_terrain},
	{"misc_skyportal_orient", SP_misc_skyportal_orient},
	{"misc_skyportal", SP_misc_skyportal},

	//rwwFIXMEFIXME: only for testing rmg team stuff
	{"gametype_item", SP_gametype_item },

	{"misc_ammo_floor_unit", SP_misc_ammo_floor_unit},
	{"misc_shield_floor_unit", SP_misc_shield_floor_unit},
	{"misc_model_shield_power_converter", SP_misc_model_shield_power_converter},
	{"misc_model_ammo_power_converter", SP_misc_model_ammo_power_converter},
	{"misc_model_health_power_converter", SP_misc_model_health_power_converter},

	{"fx_runner", SP_fx_runner},

	{"target_screenshake", SP_target_screenshake},
	{"target_escapetrig", SP_target_escapetrig},

	{"misc_maglock", SP_misc_maglock},

	{"misc_faller", SP_misc_faller},

	{"ref_tag",	SP_reference_tag},
	{"ref_tag_huge",	SP_reference_tag},

	{"misc_weapon_shooter", SP_misc_weapon_shooter},

	//new NPC ents
	{"NPC_spawner", SP_NPC_spawner},

	{"NPC_Vehicle", SP_NPC_Vehicle },
	{"NPC_Kyle", SP_NPC_Kyle },
	{"NPC_Lando", SP_NPC_Lando },
	{"NPC_Jan", SP_NPC_Jan },
	{"NPC_Luke", SP_NPC_Luke },
	{"NPC_MonMothma", SP_NPC_MonMothma },
	{"NPC_Tavion", SP_NPC_Tavion },
	
	//new tavion
	{"NPC_Tavion_New", SP_NPC_Tavion_New },

	//new alora
	{"NPC_Alora", SP_NPC_Alora },

	{"NPC_Reelo", SP_NPC_Reelo },
	{"NPC_Galak", SP_NPC_Galak },
	{"NPC_Desann", SP_NPC_Desann },
	{"NPC_Bartender", SP_NPC_Bartender },
	{"NPC_MorganKatarn", SP_NPC_MorganKatarn },
	{"NPC_Jedi", SP_NPC_Jedi },
	{"NPC_Prisoner", SP_NPC_Prisoner },
	{"NPC_Rebel", SP_NPC_Rebel },
	{"NPC_Stormtrooper", SP_NPC_Stormtrooper },
	{"NPC_StormtrooperOfficer", SP_NPC_StormtrooperOfficer },
	{"NPC_Snowtrooper", SP_NPC_Snowtrooper },
	{"NPC_Tie_Pilot", SP_NPC_Tie_Pilot },
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
	{"NPC_ShadowTrooper", SP_NPC_ShadowTrooper },
	{"NPC_Monster_Murjj", SP_NPC_Monster_Murjj },
	{"NPC_Monster_Swamp", SP_NPC_Monster_Swamp },
	{"NPC_Monster_Howler", SP_NPC_Monster_Howler },
	{"NPC_MineMonster",	SP_NPC_MineMonster },
	{"NPC_Monster_Claw", SP_NPC_Monster_Claw },
	{"NPC_Monster_Glider", SP_NPC_Monster_Glider },
	{"NPC_Monster_Flier2", SP_NPC_Monster_Flier2 },
	{"NPC_Monster_Lizard", SP_NPC_Monster_Lizard },
	{"NPC_Monster_Fish", SP_NPC_Monster_Fish },
	{"NPC_Monster_Wampa", SP_NPC_Monster_Wampa },
	{"NPC_Monster_Rancor", SP_NPC_Monster_Rancor },
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

	//maybe put these guys in some day, for now just spawn reborns in their place.
	{"NPC_Reborn_New", SP_NPC_Reborn_New },
	{"NPC_Cultist", SP_NPC_Cultist },
	{"NPC_Cultist_Saber", SP_NPC_Cultist_Saber },
	{"NPC_Cultist_Saber_Powers", SP_NPC_Cultist_Saber_Powers },
	{"NPC_Cultist_Destroyer", SP_NPC_Cultist_Destroyer },
	{"NPC_Cultist_Commando", SP_NPC_Cultist_Commando },

	//rwwFIXMEFIXME: Faked for testing NPCs (another other things) in RMG with sof2 assets
	{"NPC_Colombian_Soldier", SP_NPC_Reborn },
	{"NPC_Colombian_Rebel", SP_NPC_Reborn },
	{"NPC_Colombian_EmplacedGunner", SP_NPC_ShadowTrooper },
	{"NPC_Manuel_Vergara_RMG", SP_NPC_Desann },
//	{"info_NPCnav", SP_waypoint},

	{"waypoint", SP_waypoint},
	{"waypoint_small", SP_waypoint_small},
	{"waypoint_navgoal", SP_waypoint_navgoal},
	{"waypoint_navgoal_8", SP_waypoint_navgoal_8},
	{"waypoint_navgoal_4", SP_waypoint_navgoal_4},
	{"waypoint_navgoal_2", SP_waypoint_navgoal_2},
	{"waypoint_navgoal_1", SP_waypoint_navgoal_1},

	{"fx_spacedust", SP_CreateSpaceDust},
	{"fx_rain", SP_CreateRain},
	{"fx_snow", SP_CreateSnow},

	{"point_combat", SP_point_combat},

	{"misc_holocron", SP_misc_holocron},

	{"shooter_blaster", SP_shooter_blaster},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

	{"item_botroam", SP_item_botroam},

	{"emplaced_gun", SP_emplaced_gun},

	{"misc_turret", SP_misc_turret},
	{"misc_turretG2", SP_misc_turretG2},


	{0, 0}
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
		G_Printf ("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, ent->classname) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, ent->classname) ) {
			// found it
			if (ent->healingsound && ent->healingsound[0])
			{ //yeah...this can be used for anything, so.. precache it if it's there
				G_SoundIndex(ent->healingsound);
			}
			s->spawn(ent);
			return qtrue;
		}
	}
	G_Printf ("%s doesn't have a spawn function\n", ent->classname);
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
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
#include "../namespace_begin.h"
void BG_ParseField( BG_field_t *l_fields, const char *key, const char *value, byte *ent );
#include "../namespace_end.h"
void G_SpawnGEntityFromSpawnVars( qboolean inSubBSP ) {
	int			i;
	gentity_t	*ent;
	char		*s, *value, *gametypeName;
	static char *gametypeNames[] = {"ffa", "holocron", "jedimaster", "duel", "powerduel", "single", "team", "siege", "ctf", "cty"};

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		BG_ParseField( fields, level.spawnVars[i][0], level.spawnVars[i][1], (byte *)ent );
	}

	// check for "notsingle" flag
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_DUEL, GT_SINGLE_PLAYER)
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}

	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}

	if( G_SpawnString( "gametype", NULL, &value ) ) {
		if( g_gametype.integer >= GT_FFA && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr( value, gametypeName );
			if( !s ) {
				G_FreeEntity( ent );
				return;
			}
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}

	//Tag on the ICARUS scripting information only to valid recipients
	if ( trap_ICARUS_ValidEnt( ent ) )
	{
		trap_ICARUS_InitEnt( ent );

		if ( ent->classname && ent->classname[0] )
		{
			if ( Q_strncmp( "NPC_", ent->classname, 4 ) != 0 )
			{//Not an NPC_spawner (rww - probably don't even care for MP, but whatever)
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
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

void AddSpawnField(char *field, char *value)
{
	int	i;

	for(i=0;i<level.numSpawnVars;i++)
	{
		if (Q_stricmp(level.spawnVars[i][0], field) == 0)
		{
			level.spawnVars[ i ][1] = G_AddSpawnVarToken( value );
			return;
		}
	}

	level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( field );
	level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( value );
	level.numSpawnVars++;
}

#define NOVALUE "novalue"

static void HandleEntityAdjustment(void)
{
	char		*value;
	vec3_t		origin, newOrigin, angles;
	char		temp[MAX_QPATH];
	float		rotation;

	G_SpawnString("origin", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		sscanf( value, "%f %f %f", &origin[0], &origin[1], &origin[2] );
	}
	else
	{
		origin[0] = origin[1] = origin[2] = 0.0;
	}

	rotation = DEG2RAD(level.mRotationAdjust);
	newOrigin[0] = origin[0]*cos(rotation) - origin[1]*sin(rotation);
	newOrigin[1] = origin[0]*sin(rotation) + origin[1]*cos(rotation);
	newOrigin[2] = origin[2];
	VectorAdd(newOrigin, level.mOriginAdjust, newOrigin);
	// damn VMs don't handle outputing a float that is compatible with sscanf in all cases
	Com_sprintf(temp, MAX_QPATH, "%0.0f %0.0f %0.0f", newOrigin[0], newOrigin[1], newOrigin[2]);
	AddSpawnField("origin", temp);

	G_SpawnString("angles", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		sscanf( value, "%f %f %f", &angles[0], &angles[1], &angles[2] );

		angles[1] = fmod(angles[1] + level.mRotationAdjust, 360.0f);
		// damn VMs don't handle outputing a float that is compatible with sscanf in all cases
		Com_sprintf(temp, MAX_QPATH, "%0.0f %0.0f %0.0f", angles[0], angles[1], angles[2]);
		AddSpawnField("angles", temp);
	}
	else
	{
		G_SpawnString("angle", NOVALUE, &value);
		if (Q_stricmp(value, NOVALUE) != 0)
		{
			sscanf( value, "%f", &angles[1] );
		}
		else
		{
			angles[1] = 0.0;
		}
		angles[1] = fmod(angles[1] + level.mRotationAdjust, 360.0f);
		Com_sprintf(temp, MAX_QPATH, "%0.0f", angles[1]);
		AddSpawnField("angle", temp);
	}

	// RJR experimental code for handling "direction" field of breakable brushes
	// though direction is rarely ever used.
	G_SpawnString("direction", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		sscanf( value, "%f %f %f", &angles[0], &angles[1], &angles[2] );
	}
	else
	{
		angles[0] = angles[1] = angles[2] = 0.0;
	}
	angles[1] = fmod(angles[1] + level.mRotationAdjust, 360.0f);
	Com_sprintf(temp, MAX_QPATH, "%0.0f %0.0f %0.0f", angles[0], angles[1], angles[2]);
	AddSpawnField("direction", temp);


	AddSpawnField("BSPInstanceID", level.mTargetAdjust);

	G_SpawnString("targetname", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("targetname", temp);
	}

	G_SpawnString("target", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("target", temp);
	}

	G_SpawnString("killtarget", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("killtarget", temp);
	}

	G_SpawnString("brushparent", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("brushparent", temp);
	}

	G_SpawnString("brushchild", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("brushchild", temp);
	}

	G_SpawnString("enemy", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("enemy", temp);
	}

	G_SpawnString("ICARUSname", NOVALUE, &value);
	if (Q_stricmp(value, NOVALUE) != 0)
	{
		Com_sprintf(temp, MAX_QPATH, "%s%s", level.mTargetAdjust, value);
		AddSpawnField("ICARUSname", temp);
	}
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( qboolean inSubBSP ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {	
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}
		
		// parse value	
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	if (inSubBSP)
	{
		HandleEntityAdjustment();
	}

	return qtrue;
}


static	char *defaultStyles[32][3] = 
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

void *precachedKyle = 0;
void scriptrunner_run (gentity_t *self);

/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process

BSP Options
"gridsize"     size of lighting grid to "X Y Z". default="64 64 128"
"ambient"      scale of global light (from _color)
"fog"          shader name of the global fog texture - must include the full path, such as "textures/rj/fog1"
"distancecull" value for vis for the maximum viewing distance
"chopsize"     value for bsp on the maximum polygon / portal size
"ls_Xr"	override lightstyle X with this pattern for Red.
"ls_Xg"	green (valid patterns are "a-z")
"ls_Xb"	blue (a is OFF, z is ON)

"fogstart"		override fog start distance and force linear
"radarrange" for Siege/Vehicle radar - default range is 2500
*/
extern void EWebPrecache(void); //g_items.c
float g_cullDistance;
void SP_worldspawn( void ) 
{
	char		*text, temp[32];
	int			i;
	int			lengthRed, lengthBlue, lengthGreen;

	//I want to "cull" entities out of net sends to clients to reduce
	//net traffic on our larger open maps -rww
	G_SpawnFloat("distanceCull", "6000.0", &g_cullDistance);
	trap_SetServerCull(g_cullDistance);

	G_SpawnString( "classname", "", &text );
	if ( Q_stricmp( text, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) 
	{
		if ( Q_stricmp( "spawnscript", level.spawnVars[i][0] ) == 0 )
		{//ONly let them set spawnscript, we don't want them setting an angle or something on the world.
			BG_ParseField( fields, level.spawnVars[i][0], level.spawnVars[i][1], (byte *)&g_entities[ENTITYNUM_WORLD] );
		}
	}
	//The server will precache the standard model and animations, so that there is no hit
	//when the first client connnects.
	if (!BGPAFtextLoaded)
	{
		BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue);
	}

	if (!precachedKyle)
	{
		int defSkin;

		trap_G2API_InitGhoul2Model(&precachedKyle, "models/players/kyle/model.glm", 0, 0, -20, 0, 0);

		if (precachedKyle)
		{
			defSkin = trap_R_RegisterSkin("models/players/kyle/model_default.skin");
			trap_G2API_SetSkin(precachedKyle, 0, defSkin, defSkin);
		}
	}

	if (!g2SaberInstance)
	{
		trap_G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

		if (g2SaberInstance)
		{
			// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
			trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
			// now set up the gun bolt on it
			trap_G2API_AddBolt(g2SaberInstance, 0, "*blade1");
		}
	}

	if (g_gametype.integer == GT_SIEGE)
	{ //a tad bit of a hack, but..
		EWebPrecache();
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

	G_SpawnString( "music", "", &text );
	trap_SetConfigstring( CS_MUSIC, text );

	G_SpawnString( "message", "", &text );
	trap_SetConfigstring( CS_MESSAGE, text );				// map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );		// message of the day

	G_SpawnString( "gravity", "800", &text );
	trap_Cvar_Set( "g_gravity", text );

	G_SpawnString( "enableBreath", "0", &text );
	trap_Cvar_Set( "g_enableBreath", text );

	G_SpawnString( "soundSet", "default", &text );
	trap_SetConfigstring( CS_GLOBAL_AMBIENT_SET, text );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	} 
	/*
	else if ( g_doWarmup.integer && g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL ) { // Turn it on
		level.warmupTime = -1;
		trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
		G_LogPrintf( "Warmup:\n" );
	}
	*/

	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+0, defaultStyles[0][0]);
	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+1, defaultStyles[0][1]);
	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+2, defaultStyles[0][2]);
	
	for(i=1;i<LS_NUM_STYLES;i++)
	{
		Com_sprintf(temp, sizeof(temp), "ls_%dr", i);
		G_SpawnString(temp, defaultStyles[i][0], &text);
		lengthRed = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+0, text);

		Com_sprintf(temp, sizeof(temp), "ls_%dg", i);
		G_SpawnString(temp, defaultStyles[i][1], &text);
		lengthGreen = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+1, text);

		Com_sprintf(temp, sizeof(temp), "ls_%db", i);
		G_SpawnString(temp, defaultStyles[i][2], &text);
		lengthBlue = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+2, text);

		if (lengthRed != lengthGreen || lengthGreen != lengthBlue)
		{
			Com_Error(ERR_DROP, "Style %d has inconsistent lengths: R %d, G %d, B %d", 
				i, lengthRed, lengthGreen, lengthBlue);
		}
	}		
}

//rww - Planning on having something here?
qboolean SP_bsp_worldspawn ( void )
{
	return qtrue;
}

void G_PrecacheSoundsets( void )
{
	gentity_t	*ent = NULL;
	int i;
	int countedSets = 0;

	for ( i = 0; i < MAX_GENTITIES; i++ )
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->soundSet && ent->soundSet[0])
		{
			if (countedSets >= MAX_AMBIENT_SETS)
			{
				Com_Error(ERR_DROP, "MAX_AMBIENT_SETS was exceeded! (too many soundsets)\n");
			}

			ent->s.soundSetIndex = G_SoundSetIndex(ent->soundSet);
			countedSets++;
		}
	}
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( qboolean inSubBSP ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars(qfalse) ) {
		G_Error( "SpawnEntities: no entities" );
	}

	if (!inSubBSP)
	{
		SP_worldspawn();
	}
	else
	{
		// Skip this guy if its worldspawn fails
		if ( !SP_bsp_worldspawn() )
		{
			return;
		}
	}

	// parse ents
	while( G_ParseSpawnVars(inSubBSP) ) {
		G_SpawnGEntityFromSpawnVars(inSubBSP);
	}	

	if( g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN] && g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN][0] )
	{//World has a spawn script, but we don't want the world in ICARUS and running scripts,
		//so make a scriptrunner and start it going.
		gentity_t *script_runner = G_Spawn();
		if ( script_runner )
		{
			script_runner->behaviorSet[BSET_USE] = g_entities[ENTITYNUM_WORLD].behaviorSet[BSET_SPAWN];
			script_runner->count = 1;
			script_runner->think = scriptrunner_run;
			script_runner->nextthink = level.time + 100;

			if ( script_runner->inuse )
			{
				trap_ICARUS_InitEnt( script_runner );
			}
		}
	}

	if (!inSubBSP)
	{
		level.spawning = qfalse;			// any future calls to G_Spawn*() will be errors
	}

	G_PrecacheSoundsets();
}

