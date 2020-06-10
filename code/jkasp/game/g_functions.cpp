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

// Filename:-	g_functions.cpp
//

// This file contains the 8 (so far) function calls that replace the 8 function ptrs in the gentity_t structure

#include "g_local.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

void GEntity_ThinkFunc(gentity_t *self)
{
#define THINKCASE(blah) case thinkF_ ## blah: blah(self); break;

	switch (self->e_ThinkFunc)
	{
	case thinkF_NULL:
		break;

	THINKCASE( funcBBrushDieGo )
	THINKCASE( ExplodeDeath )
	THINKCASE( RespawnItem )
	THINKCASE( G_FreeEntity )
	THINKCASE( FinishSpawningItem )
	THINKCASE( locateCamera )
	THINKCASE( G_RunObject )
	THINKCASE( ReturnToPos1 )
	THINKCASE( Use_BinaryMover_Go )
	THINKCASE( Think_MatchTeam )
	THINKCASE( Think_BeginMoving )
	THINKCASE( Think_SetupTrainTargets )
	THINKCASE( Think_SpawnNewDoorTrigger )
	THINKCASE( ref_link )
	THINKCASE( Think_Target_Delay )
	THINKCASE( target_laser_think )
	THINKCASE( target_laser_start )
	THINKCASE( target_location_linkup )
	THINKCASE( scriptrunner_run )
	THINKCASE( multi_wait )
	THINKCASE( multi_trigger_run )
	THINKCASE( trigger_always_think )
	THINKCASE( AimAtTarget )
	THINKCASE( func_timer_think )
	THINKCASE( NPC_RemoveBody )
	THINKCASE( Disappear )
	THINKCASE( NPC_Think )
	THINKCASE( NPC_Spawn_Go )
	THINKCASE( NPC_Begin )
	THINKCASE( moverCallback )
	THINKCASE( anglerCallback )
	// This RemoveOwner need to exist here anymore???
	THINKCASE( RemoveOwner )
	THINKCASE( MakeOwnerInvis )
	THINKCASE( MakeOwnerEnergy )
	THINKCASE( func_usable_think )
	THINKCASE( misc_dlight_think )
	THINKCASE( health_think )
	THINKCASE( ammo_think )
	THINKCASE( trigger_teleporter_find_closest_portal )
	THINKCASE( thermalDetonatorExplode )
	THINKCASE( WP_ThermalThink )
	THINKCASE( trigger_hurt_reset )
	THINKCASE( turret_base_think )
	THINKCASE( turret_head_think )
	THINKCASE( laser_arm_fire )
	THINKCASE( laser_arm_start )
	THINKCASE( trigger_visible_check_player_visibility )
	THINKCASE( target_relay_use_go )
	THINKCASE( trigger_cleared_fire )
	THINKCASE( MoveOwner )
	THINKCASE( SolidifyOwner )
	THINKCASE( cycleCamera )
	THINKCASE( spawn_ammo_crystal_trigger )
	THINKCASE( NPC_ShySpawn )
	THINKCASE( func_wait_return_solid )
	THINKCASE( InflateOwner )
	THINKCASE( mega_ammo_think )
	THINKCASE( misc_replicator_item_finish_spawn )
	THINKCASE( fx_runner_link )
	THINKCASE( fx_runner_think )
	THINKCASE( fx_rain_think ) // delay flagging entities as portal entities (for sky portals)
	THINKCASE( removeBoltSurface)
	THINKCASE( set_MiscAnim)
	THINKCASE( LimbThink )
	THINKCASE( laserTrapThink )
	THINKCASE( TieFighterThink )
	THINKCASE( TieBomberThink )
	THINKCASE( rocketThink )
	THINKCASE( prox_mine_think )
	THINKCASE( emplaced_blow )
	THINKCASE( WP_Explode )
	THINKCASE( pas_think )	// personal assault sentry
	THINKCASE( ion_cannon_think )
	THINKCASE( maglock_link )
	THINKCASE( WP_flechette_alt_blow )
	THINKCASE( WP_prox_mine_think )
	THINKCASE( camera_aim )
	THINKCASE( fx_explosion_trail_link )
	THINKCASE( fx_explosion_trail_think )
	THINKCASE( fx_target_beam_link )
	THINKCASE( fx_target_beam_think )
	THINKCASE( spotlight_think )
	THINKCASE( spotlight_link )
	THINKCASE( trigger_push_checkclear )
	THINKCASE( DEMP2_AltDetonate )
	THINKCASE( DEMP2_AltRadiusDamage )
	THINKCASE( panel_turret_think )
	THINKCASE( welder_think )
	THINKCASE( gas_random_jet )
	THINKCASE( poll_converter ) // dumb loop sound handling
	THINKCASE( spawn_rack_goods ) // delay spawn of goods to help on ents
	THINKCASE( NoghriGasCloudThink )

	THINKCASE( G_PortalifyEntities ) // delay flagging entities as portal entities (for sky portals)

	THINKCASE( misc_weapon_shooter_aim )
	THINKCASE( misc_weapon_shooter_fire )

	THINKCASE( beacon_think )

	default:
		Com_Error(ERR_DROP, "GEntity_ThinkFunc: case %d not handled!\n",self->e_ThinkFunc);
		break;
	}
}

// note different switch-case code for CEntity as opposed to GEntity (CEntity goes through parent GEntity first)...
//
void CEntity_ThinkFunc(centity_s *cent)
{
#define CLTHINKCASE(blah) case clThinkF_ ## blah: blah(cent); break;

	switch (cent->gent->e_clThinkFunc)
	{
	case clThinkF_NULL:
		break;

	CLTHINKCASE( CG_DLightThink )
	CLTHINKCASE( CG_MatrixEffect )
	CLTHINKCASE( CG_Limb )

	default:
		Com_Error(ERR_DROP, "CEntity_ThinkFunc: case %d not handled!\n",cent->gent->e_clThinkFunc);
		break;
	}
}


void GEntity_ReachedFunc(gentity_t *self)
{
#define REACHEDCASE(blah) case reachedF_ ## blah: blah(self); break;

	switch (self->e_ReachedFunc)
	{
	case reachedF_NULL:
		break;

	REACHEDCASE( Reached_BinaryMover )
	REACHEDCASE( Reached_Train )
	REACHEDCASE( moverCallback )
	REACHEDCASE( moveAndRotateCallback )

	default:
		Com_Error(ERR_DROP, "GEntity_ReachedFunc: case %d not handled!\n",self->e_ReachedFunc);
		break;
	}
}



void GEntity_BlockedFunc(gentity_t *self, gentity_t *other)
{
#define BLOCKEDCASE(blah) case blockedF_ ## blah: blah(self,other); break;

	switch (self->e_BlockedFunc)
	{
	case blockedF_NULL:
		break;

	BLOCKEDCASE( Blocked_Door )
	BLOCKEDCASE( Blocked_Mover )

	default:
		Com_Error(ERR_DROP, "GEntity_BlockedFunc: case %d not handled!\n",self->e_BlockedFunc);
		break;
	}
}

void GEntity_TouchFunc(gentity_t *self, gentity_t *other, trace_t *trace)
{
#define TOUCHCASE(blah) case touchF_ ## blah: blah(self,other,trace); break;

	switch (self->e_TouchFunc)
	{
	case touchF_NULL:
		break;

	TOUCHCASE( Touch_Item )
	TOUCHCASE( teleporter_touch )
	TOUCHCASE( charge_stick )
	TOUCHCASE( Touch_DoorTrigger )
	TOUCHCASE( Touch_PlatCenterTrigger )
	TOUCHCASE( Touch_Plat )
	TOUCHCASE( Touch_Button )
	TOUCHCASE( Touch_Multi )
	TOUCHCASE( trigger_push_touch )
	TOUCHCASE( trigger_teleporter_touch )
	TOUCHCASE( hurt_touch )
	TOUCHCASE( NPC_Touch )
	TOUCHCASE( touch_ammo_crystal_tigger )
	TOUCHCASE( funcBBrushTouch )
	TOUCHCASE( touchLaserTrap )
	TOUCHCASE( prox_mine_stick )
	TOUCHCASE( func_rotating_touch )
	TOUCHCASE( TouchTieBomb )

	default:
		Com_Error(ERR_DROP, "GEntity_TouchFunc: case %d not handled!\n",self->e_TouchFunc);
	}
}

void GEntity_UseFunc(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( !self || (self->svFlags&SVF_INACTIVE) )
	{
		return;
	}
#define USECASE(blah) case useF_ ## blah: blah(self,other,activator); break;

	switch (self->e_UseFunc)
	{
	case useF_NULL:
		break;

	USECASE( funcBBrushUse )
	USECASE( misc_model_use )
	USECASE( Use_Item )
	USECASE( Use_Shooter )
	USECASE( GoExplodeDeath )
	USECASE( Use_BinaryMover )
	USECASE( use_wall )
	USECASE( Use_Target_Give )
	USECASE( Use_Target_Delay )
	USECASE( Use_Target_Score )
	USECASE( Use_Target_Print )
	USECASE( Use_Target_Speaker )
	USECASE( target_laser_use )
	USECASE( target_relay_use )
	USECASE( target_kill_use )
	USECASE( target_counter_use )
	USECASE( target_random_use )
	USECASE( target_scriptrunner_use )
	USECASE( target_gravity_change_use )
	USECASE( target_friction_change_use )
	USECASE( target_teleporter_use )
	USECASE( Use_Multi )
	USECASE( Use_target_push )
	USECASE( hurt_use )
	USECASE( func_timer_use )
	USECASE( trigger_entdist_use )
	USECASE( func_usable_use )
	USECASE( target_activate_use )
	USECASE( target_deactivate_use )
	USECASE( NPC_Use )
	USECASE( NPC_Spawn )
	USECASE( misc_dlight_use )
	USECASE( health_use )
	USECASE( ammo_use )
	USECASE( mega_ammo_use )
	USECASE( target_level_change_use )
	USECASE( target_change_parm_use )
	USECASE( turret_base_use )
	USECASE( laser_arm_use )
	USECASE( func_static_use )
	USECASE( target_play_music_use )
	USECASE( misc_model_useup )
	USECASE( misc_portal_use )
	USECASE( target_autosave_use )
	USECASE( switch_models )
	USECASE( misc_replicator_item_spawn )
	USECASE( misc_replicator_item_remove )
	USECASE( target_secret_use)
	USECASE( func_bobbing_use )
	USECASE( func_rotating_use )
	USECASE( fx_runner_use )
	USECASE( funcGlassUse )
	USECASE( TrainUse )
	USECASE( misc_trip_mine_activate )
	USECASE( emplaced_gun_use )
	USECASE( shield_power_converter_use )
	USECASE( ammo_power_converter_use )
	USECASE( bomb_planted_use )
	USECASE( beacon_use )
	USECASE( security_panel_use )
	USECASE( ion_cannon_use )
	USECASE( camera_use )
	USECASE( fx_explosion_trail_use )
	USECASE( fx_target_beam_use )
	USECASE( sentry_use )
	USECASE( spotlight_use )
	USECASE( misc_atst_use )
	USECASE( panel_turret_use )
	USECASE( welder_use )
	USECASE( jabba_cam_use )
	USECASE( misc_use )
	USECASE( pas_use )
	USECASE( item_spawn_use )
	USECASE( NPC_VehicleSpawnUse )
	USECASE( misc_weapon_shooter_use )
	USECASE( eweb_use )
	USECASE( TieFighterUse );

	default:
		Com_Error(ERR_DROP, "GEntity_UseFunc: case %d not handled!\n",self->e_UseFunc);
	}
}

void GEntity_PainFunc(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, const vec3_t point, int damage, int mod,int hitLoc)
{
#define PAINCASE(blah) case painF_ ## blah: blah(self,inflictor,attacker,point,damage,mod,hitLoc); break;

	switch (self->e_PainFunc)
	{
	case painF_NULL:
		break;

	PAINCASE( funcBBrushPain )
	PAINCASE( misc_model_breakable_pain	)
	PAINCASE( NPC_Pain )
	PAINCASE( station_pain )
	PAINCASE( func_usable_pain )
	PAINCASE( NPC_ATST_Pain )
	PAINCASE( NPC_ST_Pain )
	PAINCASE( NPC_Jedi_Pain )
	PAINCASE( NPC_Droid_Pain )
	PAINCASE( NPC_Probe_Pain )
	PAINCASE( NPC_MineMonster_Pain )
	PAINCASE( NPC_Howler_Pain )
	PAINCASE( NPC_Rancor_Pain )
	PAINCASE( NPC_Wampa_Pain )
	PAINCASE( NPC_SandCreature_Pain )
	PAINCASE( NPC_Seeker_Pain )
	PAINCASE( NPC_Remote_Pain )
	PAINCASE( emplaced_gun_pain )
	PAINCASE( NPC_Mark1_Pain )
	PAINCASE( NPC_Sentry_Pain )
	PAINCASE( NPC_Mark2_Pain )
	PAINCASE( PlayerPain )
	PAINCASE( GasBurst )
	PAINCASE( CrystalCratePain )
	PAINCASE( TurretPain )
	PAINCASE( eweb_pain )

	default:
		Com_Error(ERR_DROP, "GEntity_PainFunc: case %d not handled!\n",self->e_PainFunc);
	}
}


void GEntity_DieFunc(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc)
{
#define DIECASE(blah) case dieF_ ## blah: blah(self,inflictor,attacker,damage,mod,dFlags,hitLoc); break;

	switch (self->e_DieFunc)
	{
	case dieF_NULL:
		break;

	DIECASE( funcBBrushDie )
	DIECASE( misc_model_breakable_die )
	DIECASE( misc_model_cargo_die )
	DIECASE( func_train_die )
	DIECASE( player_die )
	DIECASE( ExplodeDeath_Wait )
	DIECASE( ExplodeDeath )
	DIECASE( func_usable_die )
	DIECASE( turret_die )
	DIECASE( funcGlassDie )
//	DIECASE( laserTrapDelayedExplode )
	DIECASE( emplaced_gun_die )
	DIECASE( WP_ExplosiveDie )
	DIECASE( ion_cannon_die )
	DIECASE( maglock_die )
	DIECASE( camera_die )
	DIECASE( Mark1_die )
	DIECASE( Interrogator_die )
	DIECASE( misc_atst_die )
	DIECASE( misc_panel_turret_die )
	DIECASE( thermal_die )
	DIECASE( eweb_die )

	default:
		Com_Error(ERR_DROP, "GEntity_DieFunc: case %d not handled!\n",self->e_DieFunc);
	}
}

//////////////////// eof /////////////////////

