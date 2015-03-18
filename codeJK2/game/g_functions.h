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

// Filename:-	g_functions.h
//

#ifndef G_FUNCTIONS
#define G_FUNCTIONS

#undef thinkFunc_t
#undef clThinkFunc_t
#undef reachedFunc_t
#undef blockedFunc_t
#undef touchFunc_t
#undef useFunc_t
#undef painFunc_t
#undef dieFunc_t

//	void		(*think)(gentity_t *self);
typedef enum 
{
	thinkF_NULL = 0,
	//
	thinkF_teleporter_think,
	thinkF_funcBBrushDieGo,
	thinkF_ExplodeDeath,
	thinkF_RespawnItem,
	thinkF_G_FreeEntity,
	thinkF_FinishSpawningItem,	
	thinkF_locateCamera,
	thinkF_G_RunObject,
	thinkF_ReturnToPos1,
	thinkF_Use_BinaryMover_Go,
	thinkF_Think_MatchTeam,
	thinkF_Think_BeginMoving,
	thinkF_Think_SetupTrainTargets,
	thinkF_Think_SpawnNewDoorTrigger,
	thinkF_ref_link,
	thinkF_Think_Target_Delay,
	thinkF_target_laser_think,
	thinkF_target_laser_start,
	thinkF_target_location_linkup,
	thinkF_scriptrunner_run,
	thinkF_multi_wait,
	thinkF_multi_trigger_run,
	thinkF_trigger_always_think,
	thinkF_AimAtTarget,
	thinkF_func_timer_think,
	thinkF_NPC_RemoveBody,
	thinkF_Disappear,
	thinkF_NPC_Think,
	thinkF_NPC_Spawn_Go,
	thinkF_NPC_Begin,
	thinkF_moverCallback,
	thinkF_anglerCallback,
	thinkF_RemoveOwner,
	thinkF_MakeOwnerInvis,
	thinkF_MakeOwnerEnergy,
	thinkF_transporter_stream_think,
	thinkF_func_usable_think,
	thinkF_misc_dlight_think,
	thinkF_health_think,
	thinkF_ammo_think,
	thinkF_trigger_teleporter_find_closest_portal,
	thinkF_thermalDetonatorExplode,
	thinkF_WP_ThermalThink,
	thinkF_trigger_hurt_reset,
	thinkF_turret_base_think,
	thinkF_turret_head_think,
	thinkF_HS_Think,
	thinkF_laser_arm_fire,
	thinkF_laser_arm_start,
	thinkF_trigger_visible_check_player_visibility,
	thinkF_target_relay_use_go,
	thinkF_trigger_cleared_fire,
	thinkF_MoveOwner,
	thinkF_SolidifyOwner,
	thinkF_cycleCamera,
	thinkF_spawn_ammo_crystal_trigger,
	thinkF_NPC_ShySpawn,
	thinkF_func_wait_return_solid,
	thinkF_InflateOwner,
	thinkF_mega_ammo_think,
	thinkF_misc_replicator_item_finish_spawn,
	thinkF_fx_runner_link,
	thinkF_fx_runner_think,
	thinkF_removeBoltSurface,
	thinkF_set_MiscAnim,
	thinkF_LimbThink,
	thinkF_laserTrapThink,
	thinkF_TieFighterThink,
	thinkF_rocketThink,
	thinkF_prox_mine_think,
	thinkF_emplaced_blow,
	thinkF_WP_Explode,
	thinkF_pas_think,	//personal assault sentry
	thinkF_ion_cannon_think,
	thinkF_maglock_link,
	thinkF_WP_flechette_alt_blow,
	thinkF_WP_prox_mine_think,
	thinkF_camera_aim,
	thinkF_fx_explosion_trail_link,
	thinkF_fx_explosion_trail_think,
	thinkF_fx_target_beam_link,
	thinkF_fx_target_beam_think,
	thinkF_spotlight_think,
	thinkF_spotlight_link,
	thinkF_trigger_push_checkclear,
	thinkF_DEMP2_AltDetonate,
	thinkF_DEMP2_AltRadiusDamage,
	thinkF_panel_turret_think,
	thinkF_welder_think,
	thinkF_gas_random_jet,
	thinkF_poll_converter,
	thinkF_spawn_rack_goods,

} thinkFunc_t;

// THINK functions...
//
extern void teleporter_think	( gentity_t *ent );
extern void funcBBrushDieGo		( gentity_t *ent );
extern void ExplodeDeath		( gentity_t *ent );
extern void RespawnItem			( gentity_t *ent );
extern void G_FreeEntity		( gentity_t *ent );
extern void FinishSpawningItem	( gentity_t *ent );
extern void locateCamera		( gentity_t *ent );
extern void G_RunObject			( gentity_t *ent );
extern void ReturnToPos1		( gentity_t *ent );
extern void Use_BinaryMover_Go	( gentity_t *ent );
extern void Think_MatchTeam		( gentity_t *ent );
extern void Think_MatchTeam		( gentity_t *ent );
extern void Think_BeginMoving	( gentity_t *ent );
extern void Think_SetupTrainTargets	( gentity_t *ent );
extern void Think_SpawnNewDoorTrigger	( gentity_t *ent );
extern void ref_link			( gentity_t *ent );
extern void Think_Target_Delay	( gentity_t *ent );
extern void target_laser_think	( gentity_t *ent );
extern void target_laser_start	( gentity_t *ent );
extern void target_location_linkup	( gentity_t *ent );
extern void scriptrunner_run	( gentity_t *ent );
extern void multi_wait			( gentity_t *ent );
extern void multi_trigger_run	( gentity_t *ent );
extern void trigger_always_think	( gentity_t *ent );
extern void AimAtTarget			( gentity_t *ent );
extern void func_timer_think	( gentity_t *ent );
extern void NPC_RemoveBody		( gentity_t *ent );
extern void Disappear			( gentity_t *ent );
extern void NPC_Think			( gentity_t *ent );
extern void NPC_Spawn_Go		( gentity_t *ent );
extern void NPC_Begin			( gentity_t *ent );
extern void moverCallback		( gentity_t *ent );
extern void anglerCallback		( gentity_t *ent );
extern void RemoveOwner			( gentity_t *ent );
extern void MakeOwnerInvis		( gentity_t *ent );
extern void MakeOwnerEnergy		( gentity_t *ent );
extern void func_usable_think	( gentity_t *self );
extern void misc_dlight_think	( gentity_t *ent ); 
extern void laser_link				( gentity_t *ent );
extern void blow_chunks_link		( gentity_t *ent );
extern void health_think			( gentity_t *ent );
extern void ammo_think				( gentity_t *ent );
extern void trigger_teleporter_find_closest_portal ( gentity_t *self );
extern void thermalDetonatorExplode	( gentity_t *ent );
extern void WP_ThermalThink			( gentity_t *ent );
extern void trigger_hurt_reset		( gentity_t *self );
extern void turret_base_think		( gentity_t *self );
extern void turret_head_think		( gentity_t *self );
extern void laser_arm_fire			( gentity_t *ent );
extern void laser_arm_start			( gentity_t *base );
extern void trigger_visible_check_player_visibility	( gentity_t *self );
extern void target_relay_use_go		( gentity_t *self );
extern void trigger_cleared_fire	( gentity_t *self );
extern void MoveOwner				( gentity_t *self );
extern void SolidifyOwner			( gentity_t *self );
extern void cycleCamera				( gentity_t *self );
extern void spawn_ammo_crystal_trigger	( gentity_t *ent );
extern void NPC_ShySpawn			( gentity_t *ent );
extern void func_wait_return_solid	( gentity_t *self );
extern void InflateOwner			( gentity_t *self );
extern void mega_ammo_think			( gentity_t *self );
extern void misc_replicator_item_finish_spawn( gentity_t *self );
extern void fx_runner_link			( gentity_t *self );
extern void fx_runner_think			( gentity_t *self );
extern void set_MiscAnim			( gentity_t *self);
extern void removeBoltSurface		( gentity_t *self);
extern void LimbThink				( gentity_t *ent );
extern void laserTrapThink			( gentity_t *self );
extern void TieFighterThink			( gentity_t *self );
extern void rocketThink				( gentity_t *ent );
extern void prox_mine_think			( gentity_t *ent );
extern void emplaced_blow			( gentity_t *self );
extern void WP_Explode				( gentity_t *self );
extern void pas_think				( gentity_t *self );
extern void ion_cannon_think		( gentity_t *self );
extern void maglock_link			( gentity_t *self );
extern void WP_flechette_alt_blow	( gentity_t *self );
extern void WP_prox_mine_think		( gentity_t *self );
extern void camera_aim				( gentity_t *self );
extern void fx_explosion_trail_link	( gentity_t *self );
extern void fx_explosion_trail_think( gentity_t *self );
extern void fx_target_beam_link		( gentity_t *self );
extern void fx_target_beam_think	( gentity_t *self );
extern void spotlight_think			( gentity_t *self );
extern void spotlight_link			( gentity_t *self );
extern void trigger_push_checkclear	( gentity_t *self );
extern void DEMP2_AltDetonate		( gentity_t *self );
extern void DEMP2_AltRadiusDamage	( gentity_t *self );
extern void panel_turret_think		( gentity_t *self );
extern void welder_think			( gentity_t *self );
extern void gas_random_jet			( gentity_t *self );
extern void poll_converter			( gentity_t *self );
extern void spawn_rack_goods		( gentity_t *self );

//	void		(*clThink)(centity_s *cent);	//Think func for equivalent centity
typedef enum
{
	clThinkF_NULL = 0,	
	//
	clThinkF_CG_DLightThink,
	clThinkF_CG_MatrixEffect,
	clThinkF_CG_Limb,

} clThinkFunc_t;

// CEntity THINK functions...
//
extern void CG_DLightThink ( centity_t *cent );
extern void CG_MatrixEffect ( centity_t *cent );
extern void CG_Limb ( centity_t *cent );

//	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
typedef enum
{
	reachedF_NULL = 0,
	//
	reachedF_Reached_BinaryMover,
	reachedF_Reached_Train,
	reachedF_moverCallback,
	reachedF_moveAndRotateCallback

} reachedFunc_t;

// REACHED functions...
//
extern void Reached_BinaryMover	( gentity_t *ent );
extern void Reached_Train		( gentity_t *ent );
extern void moverCallback		( gentity_t *ent );
extern void moveAndRotateCallback( gentity_t *ent );


//	void		(*blocked)(gentity_t *self, gentity_t *other);
typedef enum
{
	blockedF_NULL = 0,
	//
	blockedF_Blocked_Door,
	blockedF_Blocked_Mover	

} blockedFunc_t;

// BLOCKED functions...
//
extern void Blocked_Door		(gentity_t *self, gentity_t *other);
extern void Blocked_Mover		(gentity_t *self, gentity_t *other);



//	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
typedef enum
{	
	touchF_NULL = 0,
	//
	touchF_Touch_Item,
	touchF_teleporter_touch,
	touchF_charge_stick,
	touchF_Touch_DoorTrigger,
	touchF_Touch_PlatCenterTrigger,
	touchF_Touch_Plat,
	touchF_Touch_Button,
	touchF_Touch_Multi,
	touchF_trigger_push_touch,
	touchF_trigger_teleporter_touch,
	touchF_hurt_touch,
	touchF_NPC_Touch,
	touchF_touch_ammo_crystal_tigger,
	touchF_funcBBrushTouch,
	touchF_touchLaserTrap,
	touchF_prox_mine_stick,
	touchF_func_rotating_touch,

} touchFunc_t;

// TOUCH functions...
//
extern void Touch_Item				(gentity_t *self, gentity_t *other, trace_t *trace);
extern void teleporter_touch		(gentity_t *self, gentity_t *other, trace_t *trace);
extern void charge_stick			(gentity_t *self, gentity_t *other, trace_t *trace);
extern void Touch_DoorTrigger		(gentity_t *self, gentity_t *other, trace_t *trace);
extern void Touch_PlatCenterTrigger	(gentity_t *self, gentity_t *other, trace_t *trace);
extern void Touch_Plat				(gentity_t *self, gentity_t *other, trace_t *trace);
extern void Touch_Button			(gentity_t *self, gentity_t *other, trace_t *trace);
extern void Touch_Multi				(gentity_t *self, gentity_t *other, trace_t *trace);
extern void trigger_push_touch		(gentity_t *self, gentity_t *other, trace_t *trace);
extern void trigger_teleporter_touch(gentity_t *self, gentity_t *other, trace_t *trace);
extern void hurt_touch				(gentity_t *self, gentity_t *other, trace_t *trace);
extern void NPC_Touch				(gentity_t *self, gentity_t *other, trace_t *trace);
extern void touch_ammo_crystal_tigger	( gentity_t *self, gentity_t *other, trace_t *trace );
extern void funcBBrushTouch			( gentity_t *ent, gentity_t *other, trace_t *trace );
extern void touchLaserTrap	( gentity_t *ent, gentity_t *other, trace_t *trace );
extern void prox_mine_stick( gentity_t *self, gentity_t *other, trace_t *trace );
extern void func_rotating_touch				(gentity_t *self, gentity_t *other, trace_t *trace);

//	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
typedef enum
{
	useF_NULL = 0,
	//
	useF_funcBBrushUse,
	useF_misc_model_use,
	useF_Use_Item,
	useF_Use_Shooter,
	useF_GoExplodeDeath,
	useF_Use_BinaryMover,
	useF_use_wall,
	useF_Use_Target_Give,
	useF_Use_Target_Delay,
	useF_Use_Target_Score,
	useF_Use_Target_Print,
	useF_Use_Target_Speaker,
	useF_target_laser_use,
	useF_target_relay_use,
	useF_target_kill_use,
	useF_target_counter_use,
	useF_target_random_use,
	useF_target_scriptrunner_use,
	useF_target_gravity_change_use,
	useF_target_friction_change_use,
	useF_target_teleporter_use,
	useF_Use_Multi,
	useF_Use_target_push,
	useF_hurt_use,
	useF_func_timer_use,
	useF_trigger_entdist_use,
	useF_func_usable_use,
	useF_target_activate_use,
	useF_target_deactivate_use,
	useF_NPC_Use,
	useF_NPC_Spawn,
	useF_misc_dlight_use,
	useF_health_use,
	useF_ammo_use,
	useF_mega_ammo_use,
	useF_target_level_change_use,
	useF_target_change_parm_use,
	useF_crew_beam_in_use,
	useF_turret_base_use,
	useF_laser_arm_use,
	useF_func_static_use,
	useF_target_play_music_use,
	useF_misc_model_useup,
	useF_misc_portal_use,
	useF_target_autosave_use,
	useF_switch_models,
	useF_misc_replicator_item_spawn,
	useF_misc_replicator_item_remove,
	useF_target_secret_use,
	useF_func_bobbing_use,
	useF_func_rotating_use,
	useF_fx_runner_use,
	useF_funcGlassUse,
	useF_TrainUse,
	useF_misc_trip_mine_activate,
	useF_emplaced_gun_use,
	useF_shield_power_converter_use,
	useF_ammo_power_converter_use,
	useF_security_panel_use,
	useF_ion_cannon_use,
	useF_camera_use,
	useF_fx_explosion_trail_use,
	useF_fx_target_beam_use,
	useF_sentry_use,
	useF_spotlight_use,
	useF_misc_atst_use,
	useF_panel_turret_use,
	useF_welder_use,
	useF_jabba_cam_use,
	useF_misc_use,
	useF_pas_use,
	useF_item_spawn_use,

} useFunc_t;

// USE functions...
//
extern void funcBBrushUse			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void misc_model_use			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void Use_Item				( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void Use_Shooter				( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void GoExplodeDeath			( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void Use_BinaryMover			( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void use_wall				( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void Use_Target_Give			( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void Use_Target_Delay		( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void Use_Target_Score		( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void Use_Target_Print		( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void Use_Target_Speaker		( gentity_t *self, gentity_t *other, gentity_t *activator);					 
extern void target_laser_use		( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_relay_use		( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_kill_use			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_counter_use		( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_random_use		( gentity_t *self, gentity_t *other, gentity_t *activator);							
extern void target_scriptrunner_use	( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_gravity_change_use	( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_friction_change_use	( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_teleporter_use	( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void Use_Multi				( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void Use_target_push			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void hurt_use				( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void func_timer_use			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void trigger_entdist_use		( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void func_usable_use			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void target_activate_use		( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void	target_deactivate_use	( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void NPC_Use					( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void NPC_Spawn				( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void transporter_use			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void teleporter_use			( gentity_t *self, gentity_t *other, gentity_t *activator);
extern void misc_dlight_use			( gentity_t *ent, gentity_t *other, gentity_t *activator );
extern void health_use				( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void ammo_use				( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void mega_ammo_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void target_level_change_use	( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void target_change_parm_use	( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void turret_base_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void laser_arm_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void func_static_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void target_play_music_use	( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void misc_model_useup		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void misc_portal_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void target_autosave_use 	( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void switch_models			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void misc_replicator_item_spawn ( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void misc_replicator_item_remove ( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void target_secret_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void func_bobbing_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void func_rotating_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void fx_runner_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void funcGlassUse			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void TrainUse				( gentity_t *ent, gentity_t *other, gentity_t *activator );
extern void misc_trip_mine_activate	( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void emplaced_gun_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void shield_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void ammo_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void security_panel_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void ion_cannon_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void camera_use				( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void fx_explosion_trail_use	( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void fx_target_beam_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void sentry_use				( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void spotlight_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void misc_atst_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void panel_turret_use		( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void welder_use				( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void jabba_cam_use			( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void misc_use				( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void pas_use					( gentity_t *self, gentity_t *other, gentity_t *activator );
extern void item_spawn_use			( gentity_t *self, gentity_t *other, gentity_t *activator );

//	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage,int mod,int hitLoc);
typedef enum
{
	painF_NULL = 0,
	//
	painF_funcBBrushPain,
	painF_misc_model_breakable_pain,
	painF_NPC_Pain,
	painF_station_pain,
	painF_func_usable_pain,
	painF_NPC_ATST_Pain,
	painF_NPC_ST_Pain,
	painF_NPC_Jedi_Pain,
	painF_NPC_Droid_Pain,
	painF_NPC_Probe_Pain,
	painF_NPC_MineMonster_Pain,
	painF_NPC_Howler_Pain,
	painF_NPC_Seeker_Pain,
	painF_NPC_Remote_Pain,
	painF_emplaced_gun_pain,
	painF_NPC_Mark1_Pain,
	painF_NPC_GM_Pain,
	painF_NPC_Sentry_Pain,
	painF_NPC_Mark2_Pain,
	painF_PlayerPain,
	painF_GasBurst,
	painF_CrystalCratePain,
	painF_TurretPain,

} painFunc_t;

// PAIN functions...
//
extern void funcBBrushPain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void misc_model_breakable_pain	(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Pain					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void station_pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void func_usable_pain			(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_ATST_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_ST_Pain					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Jedi_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Droid_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Probe_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_MineMonster_Pain		(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Howler_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Seeker_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Remote_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void emplaced_gun_pain			(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Mark1_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_GM_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Sentry_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void NPC_Mark2_Pain				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void PlayerPain					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
extern void GasBurst					( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod, int hitLoc=HL_NONE );
extern void CrystalCratePain			( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod, int hitLoc=HL_NONE);
extern void TurretPain					( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod, int hitLoc=HL_NONE );

//	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
typedef enum
{
	dieF_NULL = 0,
	//
	dieF_funcBBrushDie,
	dieF_misc_model_breakable_die,
	dieF_misc_model_cargo_die,
	dieF_func_train_die,
	dieF_player_die,
	dieF_ExplodeDeath_Wait,
	dieF_ExplodeDeath,
	dieF_func_usable_die,
	dieF_turret_die,
	dieF_funcGlassDie,
//	dieF_laserTrapDelayedExplode,
	dieF_emplaced_gun_die,
	dieF_WP_ExplosiveDie,
	dieF_ion_cannon_die,
	dieF_maglock_die,
	dieF_camera_die,
	dieF_Mark1_die,
	dieF_Interrogator_die,
	dieF_misc_atst_die,
	dieF_misc_panel_turret_die,
	dieF_thermal_die,

} dieFunc_t;

// DIE functions...
//
extern void funcBBrushDie				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void misc_model_breakable_die	(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void misc_model_cargo_die		(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void func_train_die				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void player_die					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void ExplodeDeath_Wait			(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void ExplodeDeath				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void func_usable_die				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void turret_die					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void funcGlassDie				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void laserTrapDelayedExplode		(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void emplaced_gun_die			(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void WP_ExplosiveDie				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void ion_cannon_die				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void maglock_die					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void camera_die					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void Mark1_die					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void Interrogator_die			(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void misc_atst_die				(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void misc_panel_turret_die		(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);
extern void thermal_die					(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);


void GEntity_ThinkFunc(gentity_t *self);
void CEntity_ThinkFunc(centity_s *cent);	//Think func for equivalent centity
void GEntity_ReachedFunc(gentity_t *self);	// movers call this when hitting endpoint
void GEntity_BlockedFunc(gentity_t *self, gentity_t *other);
void GEntity_TouchFunc(gentity_t *self, gentity_t *other, trace_t *trace);
void GEntity_UseFunc(gentity_t *self, gentity_t *other, gentity_t *activator);
void GEntity_PainFunc(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc=HL_NONE);
void GEntity_DieFunc(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags=0,int hitLoc=HL_NONE);

// external functions that I now refer to...


#endif	// #ifndef G_FUNCTIONS

/////////////////// eof ///////////////////

