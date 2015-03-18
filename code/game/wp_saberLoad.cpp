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

//wp_saberLoad.cpp

#include "../qcommon/q_shared.h"
#include "g_local.h"
#include "wp_saber.h"
#include "../cgame/cg_local.h"

extern qboolean G_ParseLiteral( const char **data, const char *string );
extern saber_colors_t TranslateSaberColor( const char *name );
extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInTransition( int move );
extern qboolean PM_SaberInAttack( int move );

extern stringID_table_t FPTable[];

#define MAX_SABER_DATA_SIZE (1024*1024) // 1mb, was 512kb
char	SaberParms[MAX_SABER_DATA_SIZE];

void Saber_SithSwordPrecache( void )
{//*SIGH* special sounds used by the sith sword
	int i;
	for ( i = 1; i < 5; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/stab%d.wav", i ) );
	}
	for ( i = 1; i < 5; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/swing%d.wav", i ) );
	}
	for ( i = 1; i < 7; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/fall%d.wav", i ) );
	}
	/*
	for ( i = 1; i < 4; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/spin%d.wav", i ) );
	}
	*/
}

stringID_table_t SaberTable[] =
{
	ENUM2STRING(SABER_NONE),
	ENUM2STRING(SABER_SINGLE),
	ENUM2STRING(SABER_STAFF),
	ENUM2STRING(SABER_BROAD),
	ENUM2STRING(SABER_PRONG),
	ENUM2STRING(SABER_DAGGER),
	ENUM2STRING(SABER_ARC),
	ENUM2STRING(SABER_SAI),
	ENUM2STRING(SABER_CLAW),
	ENUM2STRING(SABER_LANCE),
	ENUM2STRING(SABER_STAR),
	ENUM2STRING(SABER_TRIDENT),
	ENUM2STRING(SABER_SITH_SWORD),
	{ "",	-1 }
};

stringID_table_t SaberMoveTable[] =
{
	ENUM2STRING(LS_NONE),
	// Attacks
	ENUM2STRING(LS_A_TL2BR),
	ENUM2STRING(LS_A_L2R),
	ENUM2STRING(LS_A_BL2TR),
	ENUM2STRING(LS_A_BR2TL),
	ENUM2STRING(LS_A_R2L),
	ENUM2STRING(LS_A_TR2BL),
	ENUM2STRING(LS_A_T2B),
	ENUM2STRING(LS_A_BACKSTAB),
	ENUM2STRING(LS_A_BACK),
	ENUM2STRING(LS_A_BACK_CR),
	ENUM2STRING(LS_ROLL_STAB),
	ENUM2STRING(LS_A_LUNGE),
	ENUM2STRING(LS_A_JUMP_T__B_),
	ENUM2STRING(LS_A_FLIP_STAB),
	ENUM2STRING(LS_A_FLIP_SLASH),
	ENUM2STRING(LS_JUMPATTACK_DUAL),
	ENUM2STRING(LS_JUMPATTACK_ARIAL_LEFT),
	ENUM2STRING(LS_JUMPATTACK_ARIAL_RIGHT),
	ENUM2STRING(LS_JUMPATTACK_CART_LEFT),
	ENUM2STRING(LS_JUMPATTACK_CART_RIGHT),
	ENUM2STRING(LS_JUMPATTACK_STAFF_LEFT),
	ENUM2STRING(LS_JUMPATTACK_STAFF_RIGHT),
	ENUM2STRING(LS_BUTTERFLY_LEFT),
	ENUM2STRING(LS_BUTTERFLY_RIGHT),
	ENUM2STRING(LS_A_BACKFLIP_ATK),
	ENUM2STRING(LS_SPINATTACK_DUAL),
	ENUM2STRING(LS_SPINATTACK),
	ENUM2STRING(LS_LEAP_ATTACK),
	ENUM2STRING(LS_SWOOP_ATTACK_RIGHT),
	ENUM2STRING(LS_SWOOP_ATTACK_LEFT),
	ENUM2STRING(LS_TAUNTAUN_ATTACK_RIGHT),
	ENUM2STRING(LS_TAUNTAUN_ATTACK_LEFT),
	ENUM2STRING(LS_KICK_F),
	ENUM2STRING(LS_KICK_B),
	ENUM2STRING(LS_KICK_R),
	ENUM2STRING(LS_KICK_L),
	ENUM2STRING(LS_KICK_S),
	ENUM2STRING(LS_KICK_BF),
	ENUM2STRING(LS_KICK_RL),
	ENUM2STRING(LS_KICK_F_AIR),
	ENUM2STRING(LS_KICK_B_AIR),
	ENUM2STRING(LS_KICK_R_AIR),
	ENUM2STRING(LS_KICK_L_AIR),
	ENUM2STRING(LS_STABDOWN),
	ENUM2STRING(LS_STABDOWN_STAFF),
	ENUM2STRING(LS_STABDOWN_DUAL),
	ENUM2STRING(LS_DUAL_SPIN_PROTECT),
	ENUM2STRING(LS_STAFF_SOULCAL),
	ENUM2STRING(LS_A1_SPECIAL),
	ENUM2STRING(LS_A2_SPECIAL),
	ENUM2STRING(LS_A3_SPECIAL),
	ENUM2STRING(LS_UPSIDE_DOWN_ATTACK),
	ENUM2STRING(LS_PULL_ATTACK_STAB),
	ENUM2STRING(LS_PULL_ATTACK_SWING),
	ENUM2STRING(LS_SPINATTACK_ALORA),
	ENUM2STRING(LS_DUAL_FB),
	ENUM2STRING(LS_DUAL_LR),
	ENUM2STRING(LS_HILT_BASH),
	{ "",	-1 }
};


saber_styles_t TranslateSaberStyle( const char *name ) {
	if ( !Q_stricmp( name, "fast" ) )		return SS_FAST;
	if ( !Q_stricmp( name, "medium" ) ) 	return SS_MEDIUM;
	if ( !Q_stricmp( name, "strong" ) ) 	return SS_STRONG;
	if ( !Q_stricmp( name, "desann" ) ) 	return SS_DESANN;
	if ( !Q_stricmp( name, "tavion" ) ) 	return SS_TAVION;
	if ( !Q_stricmp( name, "dual" ) )		return SS_DUAL;
	if ( !Q_stricmp( name, "staff" ) )		return SS_STAFF;

	return SS_NONE;
}

void WP_SaberFreeStrings( saberInfo_t &saber ) {
	if ( saber.name && gi.bIsFromZone( saber.name, TAG_G_ALLOC ) ) {
		gi.Free( saber.name );
		saber.name = NULL;
	}
	if ( saber.fullName && gi.bIsFromZone( saber.fullName, TAG_G_ALLOC ) ) {
		gi.Free( saber.fullName );
		saber.fullName = NULL;
	}
	if ( saber.model && gi.bIsFromZone( saber.model, TAG_G_ALLOC ) ) {
		gi.Free( saber.model );
		saber.model = NULL;
	}
	if ( saber.skin && gi.bIsFromZone( saber.skin, TAG_G_ALLOC ) ) {
		gi.Free( saber.skin );
		saber.skin = NULL;
	}
	if ( saber.brokenSaber1 && gi.bIsFromZone( saber.brokenSaber1, TAG_G_ALLOC ) ) {
		gi.Free( saber.brokenSaber1 );
		saber.brokenSaber1 = NULL;
	}
	if ( saber.brokenSaber2 && gi.bIsFromZone( saber.brokenSaber2, TAG_G_ALLOC ) ) {
		gi.Free( saber.brokenSaber2 );
		saber.brokenSaber2 = NULL;
	}
}

qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum ) {
	if ( saber
		&& saber->bladeStyle2Start > 0
		&& bladeNum >= saber->bladeStyle2Start )
		return qtrue;

	return qfalse;
}

qboolean WP_SaberBladeDoTransitionDamage( saberInfo_t *saber, int bladeNum ) {
	//use first blade style for this blade
	if ( !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum ) && (saber->saberFlags2 & SFL2_TRANSITION_DAMAGE) )
		return qtrue;

	//use second blade style for this blade
	else if ( WP_SaberBladeUseSecondBladeStyle( saber, bladeNum ) && (saber->saberFlags2 & SFL2_TRANSITION_DAMAGE2) )
		return qtrue;

	return qfalse;
}

qboolean WP_UseFirstValidSaberStyle( gentity_t *ent, int *saberAnimLevel )
{ 
	if ( ent && ent->client )
	{
		qboolean styleInvalid = qfalse;
		int	validStyles = 0, styleNum;

		//initially, all styles are valid
		for ( styleNum = SS_NONE+1; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
		{
			validStyles |= (1<<styleNum);
		}

		if ( ent->client->ps.saber[0].Active()
			&& ent->client->ps.saber[0].stylesForbidden )
		{
			if ( (ent->client->ps.saber[0].stylesForbidden&(1<<*saberAnimLevel)) )
			{//not a valid style for first saber!
				styleInvalid = qtrue;
				validStyles &= ~ent->client->ps.saber[0].stylesForbidden;
			}
		}
		if ( ent->client->ps.dualSabers )
		{//check second saber, too
			if ( ent->client->ps.saber[1].Active()
				&& ent->client->ps.saber[1].stylesForbidden )
			{
				if ( (ent->client->ps.saber[1].stylesForbidden&(1<<*saberAnimLevel)) )
				{//not a valid style for second saber!
					styleInvalid = qtrue;
					//only the ones both sabers allow is valid
					validStyles &= ~ent->client->ps.saber[1].stylesForbidden;
				}
			}
			else
			{//can't use dual style if not using 2 sabers
				validStyles &= ~(1<<SS_DUAL);
			}
		}
		else
		{//can't use dual style if not using 2 sabers
			validStyles &= ~(1<<SS_DUAL);
			if( *saberAnimLevel == SS_DUAL )		// saber style switch bug fixed --eez
			{
				styleInvalid = qtrue;
			}
		}
		if ( styleInvalid && validStyles )
		{//using an invalid style and have at least one valid style to use, so switch to it
			for ( styleNum = SS_NONE+1; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
			{
				if ( (validStyles&(1<<styleNum)) )
				{
					*saberAnimLevel = styleNum;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

qboolean WP_SaberStyleValidForSaber( gentity_t *ent, int saberAnimLevel )
{
	if ( ent && ent->client )
	{
		if ( ent->client->ps.saber[0].Active()
			&& ent->client->ps.saber[0].stylesForbidden )
		{
			if ( (ent->client->ps.saber[0].stylesForbidden&(1<<saberAnimLevel)) )
			{//not a valid style for first saber!
				return qfalse;
			}
		}
		if ( ent->client->ps.dualSabers )
		{//check second saber, too
			if ( ent->client->ps.saber[1].Active() )
			{
                if ( ent->client->ps.saber[1].stylesForbidden )
				{
					if ( (ent->client->ps.saber[1].stylesForbidden&(1<<saberAnimLevel)) )
					{//not a valid style for second saber!
						return qfalse;
					}
				}

				//now: if using dual sabers, only dual and tavion (if given with this saber) are allowed
				if ( saberAnimLevel != SS_DUAL )
				{//dual is okay
					if ( saberAnimLevel != SS_TAVION )
					{//tavion might be okay, all others are not
						return qfalse;
					}
					else
					{//see if "tavion" style is okay
						if ( ent->client->ps.saber[0].Active()
							&& (ent->client->ps.saber[0].stylesLearned&(1<<SS_TAVION)) )
						{//okay to use tavion style, first saber gave it to us
						}
						else if ( (ent->client->ps.saber[1].stylesLearned&(1<<SS_TAVION)) )
						{//okay to use tavion style, second saber gave it to us
						}
						else
						{//tavion style is not allowed because neither of the sabers we're using gave it to us (I know, doesn't quite make sense, but...)
							return qfalse;
						}
					}
				}
			}
			else if ( saberAnimLevel == SS_DUAL )
			{//can't use dual style if not using dualSabers
				return qfalse;
			}
		}
		else if ( saberAnimLevel == SS_DUAL )
		{//can't use dual style if not using dualSabers
			return qfalse;
		}
	}
	return qtrue;
}

qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber )
{
	if ( saber->bladeStyle2Start > 0
		&& saber->numBlades > saber->bladeStyle2Start )
	{
		if ( (saber->saberFlags2&SFL2_NO_MANUAL_DEACTIVATE)
			&& (saber->saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
		{//all blades are always on
			return qfalse;
		}
	}
	else
	{
		if ( (saber->saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
		{//all blades are always on
			return qfalse;
		}
	}
	//you can turn some off
	return qtrue;
}

void WP_SaberSetDefaults( saberInfo_t *saber, qboolean setColors = qtrue )
{
	//Set defaults so that, if it fails, there's at least something there
	saber->name = NULL;
	saber->fullName = NULL;
	for ( int i = 0; i < MAX_BLADES; i++ )
	{
		if ( setColors )
		{
			saber->blade[i].color = SABER_RED;
		}
		saber->blade[i].radius = SABER_RADIUS_STANDARD;
		saber->blade[i].lengthMax = 32;
	}
	saber->model = "models/weapons2/saber_reborn/saber_w.glm";
	saber->skin = NULL;
	saber->soundOn = G_SoundIndex( "sound/weapons/saber/enemy_saber_on.wav" );
	saber->soundLoop = G_SoundIndex( "sound/weapons/saber/saberhum3.wav" );
	saber->soundOff = G_SoundIndex( "sound/weapons/saber/enemy_saber_off.wav" );
	saber->numBlades = 1;
	saber->type = SABER_SINGLE;
	saber->stylesLearned = 0;
	saber->stylesForbidden = 0;
	saber->maxChain = 0;//0 = use default behavior
	saber->forceRestrictions = 0;
	saber->lockBonus = 0;
	saber->parryBonus = 0;
	saber->breakParryBonus = 0;
	saber->breakParryBonus2 = 0;
	saber->disarmBonus = 0;
	saber->disarmBonus2 = 0;
	saber->singleBladeStyle = SS_NONE;//makes it so that you use a different style if you only have the first blade active
	saber->brokenSaber1 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
	saber->brokenSaber2 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
//===NEW========================================================================================
	//these values are global to the saber, like all of the ones above
	saber->saberFlags = 0;					//see all the SFL_ flags
	saber->saberFlags2 = 0;					//see all the SFL2_ flags
	saber->spinSound = 0;					//none - if set, plays this sound as it spins when thrown
	saber->swingSound[0] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[1] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[2] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->fallSound[0] = 0;				//none - if set, plays one of these 3 sounds when weapon falls to the ground - NOTE: must provide all 3!!!
	saber->fallSound[1] = 0;				//none - if set, plays one of these 3 sounds when weapon falls to the ground - NOTE: must provide all 3!!!
	saber->fallSound[2] = 0;				//none - if set, plays one of these 3 sounds when weapon falls to the ground - NOTE: must provide all 3!!!

	//done in game (server-side code)
	saber->moveSpeedScale = 1.0f;				//1.0 - you move faster/slower when using this saber
	saber->animSpeedScale = 1.0f;				//1.0 - plays normal attack animations faster/slower

	saber->kataMove = LS_INVALID;				//LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time 
	saber->lungeAtkMove = LS_INVALID;			//LS_INVALID - if set, player will execute this move when they crouch+fwd+attack 
	saber->jumpAtkUpMove = LS_INVALID;			//LS_INVALID - if set, player will execute this move when they jump+attack 
	saber->jumpAtkFwdMove = LS_INVALID;			//LS_INVALID - if set, player will execute this move when they jump+fwd+attack 
	saber->jumpAtkBackMove = LS_INVALID;		//LS_INVALID - if set, player will execute this move when they jump+back+attack
	saber->jumpAtkRightMove = LS_INVALID;		//LS_INVALID - if set, player will execute this move when they jump+rightattack
	saber->jumpAtkLeftMove = LS_INVALID;		//LS_INVALID - if set, player will execute this move when they jump+left+attack
	saber->readyAnim = -1;						//-1 - anim to use when standing idle
	saber->drawAnim = -1;						//-1 - anim to use when drawing weapon
	saber->putawayAnim = -1;					//-1 - anim to use when putting weapon away
	saber->tauntAnim = -1;						//-1 - anim to use when hit "taunt"
	saber->bowAnim = -1;						//-1 - anim to use when hit "bow"
	saber->meditateAnim = -1;					//-1 - anim to use when hit "meditate"
	saber->flourishAnim = -1;					//-1 - anim to use when hit "flourish"
	saber->gloatAnim = -1;						//-1 - anim to use when hit "gloat"

	//***NOTE: you can only have a maximum of 2 "styles" of blades, so this next value, "bladeStyle2Start" is the number of the first blade to use these value on... all blades before this use the normal values above, all blades at and after this number use the secondary values below***
	saber->bladeStyle2Start = 0;			//0 - if set, blades from this number and higher use the following values (otherwise, they use the normal values already set)

	//***The following can be different for the extra blades - not setting them individually defaults them to the value for the whole saber (and first blade)***
	
	//===PRIMARY BLADES=====================
	//done in cgame (client-side code)
	saber->trailStyle = 0;					//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	saber->g2MarksShader[0]=0;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->g2WeaponMarkShader[0]=0;			//none - if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
	//saber->bladeShader = 0;				//none - if set, overrides the shader used for the saber blade?
	//saber->trailShader = 0;				//none - if set, overrides the shader used for the saber trail?
	saber->hitSound[0] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hitSound[1] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hitSound[2] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->blockSound[0] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockSound[1] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockSound[2] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->bounceSound[0] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounceSound[1] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounceSound[2] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->blockEffect = 0;					//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	saber->hitPersonEffect = 0;				//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	saber->hitOtherEffect = 0;				//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	saber->bladeEffect = 0;					//none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	saber->knockbackScale = 0;				//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	saber->damageScale = 1.0f;				//1 - scale up or down the damage done by the saber
	saber->splashRadius = 0.0f;				//0 - radius of splashDamage
	saber->splashDamage = 0;				//0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	saber->splashKnockback = 0.0f;			//0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
	
	//===SECONDARY BLADES===================
	//done in cgame (client-side code)
	saber->trailStyle2 = 0;					//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	saber->g2MarksShader2[0]=0;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->g2WeaponMarkShader2[0]=0;		//none - if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
	//saber->bladeShader = 0;				//none - if set, overrides the shader used for the saber blade?
	//saber->trailShader = 0;				//none - if set, overrides the shader used for the saber trail?
	saber->hit2Sound[0] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hit2Sound[1] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hit2Sound[2] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->block2Sound[0] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->block2Sound[1] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->block2Sound[2] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->bounce2Sound[0] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounce2Sound[1] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounce2Sound[2] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->blockEffect2 = 0;					//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	saber->hitPersonEffect2 = 0;				//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	saber->hitOtherEffect2 = 0;				//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	saber->bladeEffect2 = 0;				//none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	saber->knockbackScale2 = 0;				//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	saber->damageScale2 = 1.0f;				//1 - scale up or down the damage done by the saber
	saber->splashRadius2 = 0.0f;				//0 - radius of splashDamage
	saber->splashDamage2 = 0;				//0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	saber->splashKnockback2 = 0.0f;			//0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
//=========================================================================================================================================
}

static void Saber_ParseName( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->fullName = G_NewString( value );
}
static void Saber_ParseSaberType( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberType;
	if ( COM_ParseString( p, &value ) )
		return;
	saberType = GetIDForString( SaberTable, value );
	if ( saberType >= SABER_SINGLE && saberType <= NUM_SABERS )
		saber->type = (saberType_t)saberType;
}
static void Saber_ParseSaberModel( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->model = G_NewString( value );
}
static void Saber_ParseCustomSkin( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->skin = G_NewString( value );
}
static void Saber_ParseSoundOn( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->soundOn = G_SoundIndex( value );
}
static void Saber_ParseSoundLoop( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->soundLoop = G_SoundIndex( value );
}
static void Saber_ParseSoundOff( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->soundOff = G_SoundIndex( value );
}
static void Saber_ParseNumBlades( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n < 1 || n > MAX_BLADES ) {
		Com_Error( ERR_DROP, "WP_SaberParseParms: saber %s has illegal number of blades (%d) max: %d", saber->name, n, MAX_BLADES );
		return;
	}
	saber->numBlades = n;
}
qboolean Saber_SetColor = qtrue;
static void Saber_ParseSaberColor( saberInfo_t *saber, const char **p ) {
	const char *value;
	int i=0;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	for ( i=0; i<MAX_BLADES; i++ )
		saber->blade[i].color = color;
}
static void Saber_ParseSaberColor2( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	saber->blade[1].color = color;
}
static void Saber_ParseSaberColor3( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	saber->blade[2].color = color;
}
static void Saber_ParseSaberColor4( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	saber->blade[3].color = color;
}
static void Saber_ParseSaberColor5( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	saber->blade[4].color = color;
}
static void Saber_ParseSaberColor6( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	saber->blade[5].color = color;
}
static void Saber_ParseSaberColor7( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	// don't actually want to set the colors
	// read the color out anyway just to advance the *p pointer
	if ( !Saber_SetColor )
		return;

	color = TranslateSaberColor( value );
	saber->blade[6].color = color;
}
static void Saber_ParseSaberLength( saberInfo_t *saber, const char **p ) {
	int i=0;
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	for ( i=0; i<MAX_BLADES; i++ )
		saber->blade[i].lengthMax = f;
}
static void Saber_ParseSaberLength2( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	saber->blade[1].lengthMax = f;
}
static void Saber_ParseSaberLength3( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	saber->blade[2].lengthMax = f;
}
static void Saber_ParseSaberLength4( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	saber->blade[3].lengthMax = f;
}
static void Saber_ParseSaberLength5( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	saber->blade[4].lengthMax = f;
}
static void Saber_ParseSaberLength6( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	saber->blade[5].lengthMax = f;
}
static void Saber_ParseSaberLength7( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 4.0f )
		f = 4.0f;

	saber->blade[6].lengthMax = f;
}
static void Saber_ParseSaberRadius( saberInfo_t *saber, const char **p ) {
	int i=0;
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	for ( i=0; i<MAX_BLADES; i++ )
		saber->blade[i].radius = f;
}
static void Saber_ParseSaberRadius2( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	saber->blade[1].radius = f;
}
static void Saber_ParseSaberRadius3( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	saber->blade[2].radius = f;
}
static void Saber_ParseSaberRadius4( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	saber->blade[3].radius = f;
}
static void Saber_ParseSaberRadius5( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	saber->blade[4].radius = f;
}
static void Saber_ParseSaberRadius6( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	saber->blade[5].radius = f;
}
static void Saber_ParseSaberRadius7( saberInfo_t *saber, const char **p ) {
	float f;

	if ( COM_ParseFloat( p, &f ) )
		return;

	if ( f < 0.25f )
		f = 0.25f;

	saber->blade[6].radius = f;
}
static void Saber_ParseSaberStyle( saberInfo_t *saber, const char **p ) {
	const char *value;
	int style, styleNum;

	if ( COM_ParseString( p, &value ) )
		return;

	//OLD WAY: only allowed ONE style
	style = TranslateSaberStyle( value );
	//learn only this style
	saber->stylesLearned = (1<<style);
	//forbid all other styles
	saber->stylesForbidden = 0;
	for ( styleNum=SS_NONE+1; styleNum<SS_NUM_SABER_STYLES; styleNum++ ) {
		if ( styleNum != style )
			saber->stylesForbidden |= (1<<styleNum);
	}
}
static void Saber_ParseSaberStyleLearned( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->stylesLearned |= (1<<TranslateSaberStyle( value ));
}
static void Saber_ParseSaberStyleForbidden( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->stylesForbidden |= (1<<TranslateSaberStyle( value ));
}
static void Saber_ParseMaxChain( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->maxChain = n;
}
static void Saber_ParseLockable( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n == 0 )
		saber->saberFlags |= SFL_NOT_LOCKABLE;
}
static void Saber_ParseThrowable( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n == 0 )
		saber->saberFlags |= SFL_NOT_THROWABLE;
}
static void Saber_ParseDisarmable( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n == 0 )
		saber->saberFlags |= SFL_NOT_DISARMABLE;
}
static void Saber_ParseBlocking( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n == 0 )
		saber->saberFlags |= SFL_NOT_ACTIVE_BLOCKING;
}
static void Saber_ParseTwoHanded( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_TWO_HANDED;
}
static void Saber_ParseForceRestrict( saberInfo_t *saber, const char **p ) {
	const char *value;
	int fp;

	if ( COM_ParseString( p, &value ) )
		return;

	fp = GetIDForString( FPTable, value );
	if ( fp >= FP_FIRST && fp < NUM_FORCE_POWERS )
		saber->forceRestrictions |= (1<<fp);
}
static void Saber_ParseLockBonus( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->lockBonus = n;
}
static void Saber_ParseParryBonus( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->parryBonus = n;
}
static void Saber_ParseBreakParryBonus( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->breakParryBonus = n;
}
static void Saber_ParseBreakParryBonus2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->breakParryBonus2 = n;
}
static void Saber_ParseDisarmBonus( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->disarmBonus = n;
}
static void Saber_ParseDisarmBonus2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->disarmBonus2 = n;
}
static void Saber_ParseSingleBladeStyle( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->singleBladeStyle = TranslateSaberStyle( value );
}
static void Saber_ParseSingleBladeThrowable( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_SINGLE_BLADE_THROWABLE;
}
static void Saber_ParseBrokenSaber1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->brokenSaber1 = G_NewString( value );
}
static void Saber_ParseBrokenSaber2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->brokenSaber2 = G_NewString( value );
}
static void Saber_ParseReturnDamage( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_RETURN_DAMAGE;
}
static void Saber_ParseSpinSound( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->spinSound = G_SoundIndex( value );
}
static void Saber_ParseSwingSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->swingSound[0] = G_SoundIndex( value );
}
static void Saber_ParseSwingSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->swingSound[1] = G_SoundIndex( value );
}
static void Saber_ParseSwingSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->swingSound[2] = G_SoundIndex( value );
}
static void Saber_ParseFallSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->fallSound[0] = G_SoundIndex( value );
}
static void Saber_ParseFallSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->fallSound[1] = G_SoundIndex( value );
}
static void Saber_ParseFallSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->fallSound[2] = G_SoundIndex( value );
}
static void Saber_ParseMoveSpeedScale( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->moveSpeedScale = f;
}
static void Saber_ParseAnimSpeedScale( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->animSpeedScale = f;
}
static void Saber_ParseOnInWater( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_ON_IN_WATER;
}
static void Saber_ParseBounceOnWalls( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_BOUNCE_ON_WALLS;
}
static void Saber_ParseBoltToWrist( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_BOLT_TO_WRIST;
}
static void Saber_ParseKataMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->kataMove = saberMove; //LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time
}
static void Saber_ParseLungeAtkMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->lungeAtkMove = saberMove;
}
static void Saber_ParseJumpAtkUpMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkUpMove = saberMove;
}
static void Saber_ParseJumpAtkFwdMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkFwdMove = saberMove;
}
static void Saber_ParseJumpAtkBackMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkBackMove = saberMove;
}
static void Saber_ParseJumpAtkRightMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkRightMove = saberMove;
}
static void Saber_ParseJumpAtkLeftMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( SaberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkLeftMove = saberMove;
}
static void Saber_ParseReadyAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->readyAnim = anim;
}
static void Saber_ParseDrawAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->drawAnim = anim;
}
static void Saber_ParsePutawayAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->putawayAnim = anim;
}
static void Saber_ParseTauntAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->tauntAnim = anim;
}
static void Saber_ParseBowAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;

	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->bowAnim = anim;
}
static void Saber_ParseMeditateAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->meditateAnim = anim;
}
static void Saber_ParseFlourishAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->flourishAnim = anim;
}
static void Saber_ParseGloatAnim( saberInfo_t *saber, const char **p ) {
	const char *value;
	int anim = -1;
	if ( COM_ParseString( p, &value ) )
		return;
	anim = GetIDForString( animTable, value );
	if ( anim >= 0 && anim < MAX_ANIMATIONS )
		saber->gloatAnim = anim;
}
static void Saber_ParseNoRollStab( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_ROLL_STAB;
}
static void Saber_ParseNoPullAttack( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_PULL_ATTACK;
}
static void Saber_ParseNoBackAttack( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_BACK_ATTACK;
}
static void Saber_ParseNoStabDown( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_STABDOWN;
}
static void Saber_ParseNoWallRuns( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_WALL_RUNS;
}
static void Saber_ParseNoWallFlips( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_WALL_FLIPS;
}
static void Saber_ParseNoWallGrab( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_WALL_GRAB;
}
static void Saber_ParseNoRolls( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_ROLLS;
}
static void Saber_ParseNoFlips( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_FLIPS;
}
static void Saber_ParseNoCartwheels( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_CARTWHEELS;
}
static void Saber_ParseNoKicks( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_KICKS;
}
static void Saber_ParseNoMirrorAttacks( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags |= SFL_NO_MIRROR_ATTACKS;
}
static void Saber_ParseNotInMP( saberInfo_t *saber, const char **p ) {
	SkipRestOfLine( p );
}
static void Saber_ParseBladeStyle2Start( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->bladeStyle2Start = n;
}
static void Saber_ParseNoWallMarks( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_WALL_MARKS;
}
static void Saber_ParseNoDLight( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_DLIGHT;
}
static void Saber_ParseNoBlade( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_BLADE;
}
static void Saber_ParseTrailStyle( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->trailStyle = n;
}
static void Saber_ParseG2MarksShader( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) ) {
		SkipRestOfLine( p );
		return;
	}
	Q_strncpyz( saber->g2MarksShader, value, sizeof( saber->g2MarksShader ), qtrue );
	//NOTE: registers this on cgame side where it registers all client assets
}
static void Saber_ParseG2WeaponMarkShader( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) ) {
		SkipRestOfLine( p );
		return;
	}
	Q_strncpyz( saber->g2WeaponMarkShader, value, sizeof( saber->g2WeaponMarkShader ), qtrue );
	//NOTE: registers this on cgame side where it registers all client assets
}
static void Saber_ParseKnockbackScale( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->knockbackScale = f;
}
static void Saber_ParseDamageScale( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->damageScale = f;
}
static void Saber_ParseNoDismemberment( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_DISMEMBERMENT;
}
static void Saber_ParseNoIdleEffect( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_IDLE_EFFECT;
}
static void Saber_ParseAlwaysBlock( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_ALWAYS_BLOCK;
}
static void Saber_ParseNoManualDeactivate( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_MANUAL_DEACTIVATE;
}
static void Saber_ParseTransitionDamage( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_TRANSITION_DAMAGE;
}
static void Saber_ParseSplashRadius( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->splashRadius = f;
}
static void Saber_ParseSplashDamage( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->splashDamage = n;
}
static void Saber_ParseSplashKnockback( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->splashKnockback = f;
}
static void Saber_ParseHitSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitSound[0] = G_SoundIndex( value );
}
static void Saber_ParseHitSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitSound[1] = G_SoundIndex( value );
}
static void Saber_ParseHitSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitSound[2] = G_SoundIndex( value );
}
static void Saber_ParseBlockSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockSound[0] = G_SoundIndex( value );
}
static void Saber_ParseBlockSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockSound[1] = G_SoundIndex( value );
}
static void Saber_ParseBlockSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockSound[2] = G_SoundIndex( value );
}
static void Saber_ParseBounceSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounceSound[0] = G_SoundIndex( value );
}
static void Saber_ParseBounceSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounceSound[1] = G_SoundIndex( value );
}
static void Saber_ParseBounceSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounceSound[2] = G_SoundIndex( value );
}
static void Saber_ParseBlockEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockEffect = G_EffectIndex( value );
}
static void Saber_ParseHitPersonEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitPersonEffect = G_EffectIndex( value );
}
static void Saber_ParseHitOtherEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitOtherEffect = G_EffectIndex( value );
}
static void Saber_ParseBladeEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bladeEffect = G_EffectIndex( value );
}
static void Saber_ParseNoClashFlare( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_CLASH_FLARE;
}
static void Saber_ParseNoWallMarks2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_WALL_MARKS2;
}
static void Saber_ParseNoDLight2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_DLIGHT2;
}
static void Saber_ParseNoBlade2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_BLADE2;
}
static void Saber_ParseTrailStyle2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->trailStyle2 = n;
}
static void Saber_ParseG2MarksShader2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) ) {
		SkipRestOfLine( p );
		return;
	}
	Q_strncpyz( saber->g2MarksShader2, value, sizeof(saber->g2MarksShader2), qtrue );
	//NOTE: registers this on cgame side where it registers all client assets
}
static void Saber_ParseG2WeaponMarkShader2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) ) {
		SkipRestOfLine( p );
		return;
	}
	Q_strncpyz( saber->g2WeaponMarkShader2, value, sizeof(saber->g2WeaponMarkShader2), qtrue );
	//NOTE: registers this on cgame side where it registers all client assets
}
static void Saber_ParseKnockbackScale2( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->knockbackScale2 = f;
}
static void Saber_ParseDamageScale2( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->damageScale2 = f;
}
static void Saber_ParseNoDismemberment2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_DISMEMBERMENT2;
}
static void Saber_ParseNoIdleEffect2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_IDLE_EFFECT2;
}
static void Saber_ParseAlwaysBlock2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_ALWAYS_BLOCK2;
}
static void Saber_ParseNoManualDeactivate2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_MANUAL_DEACTIVATE2;
}
static void Saber_ParseTransitionDamage2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_TRANSITION_DAMAGE2;
}
static void Saber_ParseSplashRadius2( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->splashRadius2 = f;
}
static void Saber_ParseSplashDamage2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->splashDamage2 = n;
}
static void Saber_ParseSplashKnockback2( saberInfo_t *saber, const char **p ) {
	float f;
	if ( COM_ParseFloat( p, &f ) ) {
		SkipRestOfLine( p );
		return;
	}
	saber->splashKnockback2 = f;
}
static void Saber_ParseHit2Sound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hit2Sound[0] = G_SoundIndex( value );
}
static void Saber_ParseHit2Sound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hit2Sound[1] = G_SoundIndex( value );
}
static void Saber_ParseHit2Sound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hit2Sound[2] = G_SoundIndex( value );
}
static void Saber_ParseBlock2Sound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->block2Sound[0] = G_SoundIndex( value );
}
static void Saber_ParseBlock2Sound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->block2Sound[1] = G_SoundIndex( value );
}
static void Saber_ParseBlock2Sound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->block2Sound[2] = G_SoundIndex( value );
}
static void Saber_ParseBounce2Sound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounce2Sound[0] = G_SoundIndex( value );
}
static void Saber_ParseBounce2Sound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounce2Sound[1] = G_SoundIndex( value );
}
static void Saber_ParseBounce2Sound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounce2Sound[2] = G_SoundIndex( value );
}
static void Saber_ParseBlockEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockEffect2 = G_EffectIndex( value );
}
static void Saber_ParseHitPersonEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitPersonEffect2 = G_EffectIndex( value );
}
static void Saber_ParseHitOtherEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitOtherEffect2 = G_EffectIndex( value );
}
static void Saber_ParseBladeEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bladeEffect2 = G_EffectIndex( (char *)value );
}
static void Saber_ParseNoClashFlare2( saberInfo_t *saber, const char **p ) {
	int n;
	if ( COM_ParseInt( p, &n ) ) {
		SkipRestOfLine( p );
		return;
	}
	if ( n )
		saber->saberFlags2 |= SFL2_NO_CLASH_FLARE2;
}

/*
===============
Keyword Hash
===============
*/

#define KEYWORDHASH_SIZE (512)

typedef struct keywordHash_s {
	char	*keyword;
	void	(*func)(saberInfo_t *saber, const char **p);

	struct keywordHash_s *next;
} keywordHash_t;

static int KeywordHash_Key( const char *keyword ) {
	int register hash, i;

	hash = 0;
	for ( i=0; keyword[i]; i++ ) {
		if ( keyword[i] >= 'A' && keyword[i] <= 'Z' )
			hash += (keyword[i] + ('a'-'A')) * (119 + i);
		else
			hash += keyword[i] * (119 + i);
	}

	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (KEYWORDHASH_SIZE-1);
	return hash;
}

static void KeywordHash_Add( keywordHash_t *table[], keywordHash_t *key ) {
	int hash = KeywordHash_Key( key->keyword );

	key->next = table[hash];
	table[hash] = key;
}

static keywordHash_t *KeywordHash_Find( keywordHash_t *table[], const char *keyword ) {
	keywordHash_t *key;
	int hash = KeywordHash_Key(keyword);

	for ( key=table[hash]; key; key=key->next ) {
		if ( !Q_stricmp( key->keyword, keyword ) )
			return key;
	}

	return NULL;
}

static keywordHash_t saberParseKeywords[] = {
	{ "name",					Saber_ParseName,				NULL	},
	{ "saberType",				Saber_ParseSaberType,			NULL	},
	{ "saberModel",				Saber_ParseSaberModel,			NULL	},
	{ "customSkin",				Saber_ParseCustomSkin,			NULL	},
	{ "soundOn",				Saber_ParseSoundOn,				NULL	},
	{ "soundLoop",				Saber_ParseSoundLoop,			NULL	},
	{ "soundOff",				Saber_ParseSoundOff,			NULL	},
	{ "numBlades",				Saber_ParseNumBlades,			NULL	},
	{ "saberColor",				Saber_ParseSaberColor,			NULL	},
	{ "saberColor2",			Saber_ParseSaberColor2,			NULL	},
	{ "saberColor3",			Saber_ParseSaberColor3,			NULL	},
	{ "saberColor4",			Saber_ParseSaberColor4,			NULL	},
	{ "saberColor5",			Saber_ParseSaberColor5,			NULL	},
	{ "saberColor6",			Saber_ParseSaberColor6,			NULL	},
	{ "saberColor7",			Saber_ParseSaberColor7,			NULL	},
	{ "saberLength",			Saber_ParseSaberLength,			NULL	},
	{ "saberLength2",			Saber_ParseSaberLength2,		NULL	},
	{ "saberLength3",			Saber_ParseSaberLength3,		NULL	},
	{ "saberLength4",			Saber_ParseSaberLength4,		NULL	},
	{ "saberLength5",			Saber_ParseSaberLength5,		NULL	},
	{ "saberLength6",			Saber_ParseSaberLength6,		NULL	},
	{ "saberLength7",			Saber_ParseSaberLength7,		NULL	},
	{ "saberRadius",			Saber_ParseSaberRadius,			NULL	},
	{ "saberRadius2",			Saber_ParseSaberRadius2,		NULL	},
	{ "saberRadius3",			Saber_ParseSaberRadius3,		NULL	},
	{ "saberRadius4",			Saber_ParseSaberRadius4,		NULL	},
	{ "saberRadius5",			Saber_ParseSaberRadius5,		NULL	},
	{ "saberRadius6",			Saber_ParseSaberRadius6,		NULL	},
	{ "saberRadius7",			Saber_ParseSaberRadius7,		NULL	},
	{ "saberStyle",				Saber_ParseSaberStyle,			NULL	},
	{ "saberStyleLearned",		Saber_ParseSaberStyleLearned,	NULL	},
	{ "saberStyleForbidden",	Saber_ParseSaberStyleForbidden,	NULL	},
	{ "maxChain",				Saber_ParseMaxChain,			NULL	},
	{ "lockable",				Saber_ParseLockable,			NULL	},
	{ "throwable",				Saber_ParseThrowable,			NULL	},
	{ "disarmable",				Saber_ParseDisarmable,			NULL	},
	{ "blocking",				Saber_ParseBlocking,			NULL	},
	{ "twoHanded",				Saber_ParseTwoHanded,			NULL	},
	{ "forceRestrict",			Saber_ParseForceRestrict,		NULL	},
	{ "lockBonus",				Saber_ParseLockBonus,			NULL	},
	{ "parryBonus",				Saber_ParseParryBonus,			NULL	},
	{ "breakParryBonus",		Saber_ParseBreakParryBonus,		NULL	},
	{ "breakParryBonus2",		Saber_ParseBreakParryBonus2,	NULL	},
	{ "disarmBonus",			Saber_ParseDisarmBonus,			NULL	},
	{ "disarmBonus2",			Saber_ParseDisarmBonus2,		NULL	},
	{ "singleBladeStyle",		Saber_ParseSingleBladeStyle,	NULL	},
	{ "singleBladeThrowable",	Saber_ParseSingleBladeThrowable,NULL	},
	{ "brokenSaber1",			Saber_ParseBrokenSaber1,		NULL	},
	{ "brokenSaber2",			Saber_ParseBrokenSaber2,		NULL	},
	{ "returnDamage",			Saber_ParseReturnDamage,		NULL	},
	{ "spinSound",				Saber_ParseSpinSound,			NULL	},
	{ "swingSound1",			Saber_ParseSwingSound1,			NULL	},
	{ "swingSound2",			Saber_ParseSwingSound2,			NULL	},
	{ "swingSound3",			Saber_ParseSwingSound3,			NULL	},
	{ "fallSound1",				Saber_ParseFallSound1,			NULL	},
	{ "fallSound2",				Saber_ParseFallSound2,			NULL	},
	{ "fallSound3",				Saber_ParseFallSound3,			NULL	},
	{ "moveSpeedScale",			Saber_ParseMoveSpeedScale,		NULL	},
	{ "animSpeedScale",			Saber_ParseAnimSpeedScale,		NULL	},
	{ "onInWater",				Saber_ParseOnInWater,			NULL	},
	{ "bounceOnWalls",			Saber_ParseBounceOnWalls,		NULL	},
	{ "boltToWrist",			Saber_ParseBoltToWrist,			NULL	},
	{ "kataMove",				Saber_ParseKataMove,			NULL	},
	{ "lungeAtkMove",			Saber_ParseLungeAtkMove,		NULL	},
	{ "jumpAtkUpMove",			Saber_ParseJumpAtkUpMove,		NULL	},
	{ "jumpAtkFwdMove",			Saber_ParseJumpAtkFwdMove,		NULL	},
	{ "jumpAtkBackMove",		Saber_ParseJumpAtkBackMove,		NULL	},
	{ "jumpAtkRightMove",		Saber_ParseJumpAtkRightMove,	NULL	},
	{ "jumpAtkLeftMove",		Saber_ParseJumpAtkLeftMove,		NULL	},
	{ "readyAnim",				Saber_ParseReadyAnim,			NULL	},
	{ "drawAnim",				Saber_ParseDrawAnim,			NULL	},
	{ "putawayAnim",			Saber_ParsePutawayAnim,			NULL	},
	{ "tauntAnim",				Saber_ParseTauntAnim,			NULL	},
	{ "bowAnim",				Saber_ParseBowAnim,				NULL	},
	{ "meditateAnim",			Saber_ParseMeditateAnim,		NULL	},
	{ "flourishAnim",			Saber_ParseFlourishAnim,		NULL	},
	{ "gloatAnim",				Saber_ParseGloatAnim,			NULL	},
	{ "noRollStab",				Saber_ParseNoRollStab,			NULL	},
	{ "noPullAttack",			Saber_ParseNoPullAttack,		NULL	},
	{ "noBackAttack",			Saber_ParseNoBackAttack,		NULL	},
	{ "noStabDown",				Saber_ParseNoStabDown,			NULL	},
	{ "noWallRuns",				Saber_ParseNoWallRuns,			NULL	},
	{ "noWallFlips",			Saber_ParseNoWallFlips,			NULL	},
	{ "noWallGrab",				Saber_ParseNoWallGrab,			NULL	},
	{ "noRolls",				Saber_ParseNoRolls,				NULL	},
	{ "noFlips",				Saber_ParseNoFlips,				NULL	},
	{ "noCartwheels",			Saber_ParseNoCartwheels,		NULL	},
	{ "noKicks",				Saber_ParseNoKicks,				NULL	},
	{ "noMirrorAttacks",		Saber_ParseNoMirrorAttacks,		NULL	},
	{ "notInMP",				Saber_ParseNotInMP,				NULL	},
	{ "bladeStyle2Start",		Saber_ParseBladeStyle2Start,	NULL	},
	{ "noWallMarks",			Saber_ParseNoWallMarks,			NULL	},
	{ "noWallMarks2",			Saber_ParseNoWallMarks2,		NULL	},
	{ "noDlight",				Saber_ParseNoDLight,			NULL	},
	{ "noDlight2",				Saber_ParseNoDLight2,			NULL	},
	{ "noBlade",				Saber_ParseNoBlade,				NULL	},
	{ "noBlade2",				Saber_ParseNoBlade2,			NULL	},
	{ "trailStyle",				Saber_ParseTrailStyle,			NULL	},
	{ "trailStyle2",			Saber_ParseTrailStyle2,			NULL	},
	{ "g2MarksShader",			Saber_ParseG2MarksShader,		NULL	},
	{ "g2MarksShader2",			Saber_ParseG2MarksShader2,		NULL	},
	{ "g2WeaponMarkShader",		Saber_ParseG2WeaponMarkShader,	NULL	},
	{ "g2WeaponMarkShader2",	Saber_ParseG2WeaponMarkShader2,	NULL	},
	{ "knockbackScale",			Saber_ParseKnockbackScale,		NULL	},
	{ "knockbackScale2",		Saber_ParseKnockbackScale2,		NULL	},
	{ "damageScale",			Saber_ParseDamageScale,			NULL	},
	{ "damageScale2",			Saber_ParseDamageScale2,		NULL	},
	{ "noDismemberment",		Saber_ParseNoDismemberment,		NULL	},
	{ "noDismemberment2",		Saber_ParseNoDismemberment2,	NULL	},
	{ "noIdleEffect",			Saber_ParseNoIdleEffect,		NULL	},
	{ "noIdleEffect2",			Saber_ParseNoIdleEffect2,		NULL	},
	{ "alwaysBlock",			Saber_ParseAlwaysBlock,			NULL	},
	{ "alwaysBlock2",			Saber_ParseAlwaysBlock2,		NULL	},
	{ "noManualDeactivate",		Saber_ParseNoManualDeactivate,	NULL	},
	{ "noManualDeactivate2",	Saber_ParseNoManualDeactivate2,	NULL	},
	{ "transitionDamage",		Saber_ParseTransitionDamage,	NULL	},
	{ "transitionDamage2",		Saber_ParseTransitionDamage2,	NULL	},
	{ "splashRadius",			Saber_ParseSplashRadius,		NULL	},
	{ "splashRadius2",			Saber_ParseSplashRadius2,		NULL	},
	{ "splashDamage",			Saber_ParseSplashDamage,		NULL	},
	{ "splashDamage2",			Saber_ParseSplashDamage2,		NULL	},
	{ "splashKnockback",		Saber_ParseSplashKnockback,		NULL	},
	{ "splashKnockback2",		Saber_ParseSplashKnockback2,	NULL	},
	{ "hitSound1",				Saber_ParseHitSound1,			NULL	},
	{ "hit2Sound1",				Saber_ParseHit2Sound1,			NULL	},
	{ "hitSound2",				Saber_ParseHitSound2,			NULL	},
	{ "hit2Sound2",				Saber_ParseHit2Sound2,			NULL	},
	{ "hitSound3",				Saber_ParseHitSound3,			NULL	},
	{ "hit2Sound3",				Saber_ParseHit2Sound3,			NULL	},
	{ "blockSound1",			Saber_ParseBlockSound1,			NULL	},
	{ "block2Sound1",			Saber_ParseBlock2Sound1,		NULL	},
	{ "blockSound2",			Saber_ParseBlockSound2,			NULL	},
	{ "block2Sound2",			Saber_ParseBlock2Sound2,		NULL	},
	{ "blockSound3",			Saber_ParseBlockSound3,			NULL	},
	{ "block2Sound3",			Saber_ParseBlock2Sound3,		NULL	},
	{ "bounceSound1",			Saber_ParseBounceSound1,		NULL	},
	{ "bounce2Sound1",			Saber_ParseBounce2Sound1,		NULL	},
	{ "bounceSound2",			Saber_ParseBounceSound2,		NULL	},
	{ "bounce2Sound2",			Saber_ParseBounce2Sound2,		NULL	},
	{ "bounceSound3",			Saber_ParseBounceSound3,		NULL	},
	{ "bounce2Sound3",			Saber_ParseBounce2Sound3,		NULL	},
	{ "blockEffect",			Saber_ParseBlockEffect,			NULL	},
	{ "blockEffect2",			Saber_ParseBlockEffect2,		NULL	},
	{ "hitPersonEffect",		Saber_ParseHitPersonEffect,		NULL	},
	{ "hitPersonEffect2",		Saber_ParseHitPersonEffect2,	NULL	},
	{ "hitOtherEffect",			Saber_ParseHitOtherEffect,		NULL	},
	{ "hitOtherEffect2",		Saber_ParseHitOtherEffect2,		NULL	},
	{ "bladeEffect",			Saber_ParseBladeEffect,			NULL	},
	{ "bladeEffect2",			Saber_ParseBladeEffect2,		NULL	},
	{ "noClashFlare",			Saber_ParseNoClashFlare,		NULL	},
	{ "noClashFlare2",			Saber_ParseNoClashFlare2,		NULL	},
	{ NULL,						NULL,							NULL	}
};
static keywordHash_t *saberParseKeywordHash[KEYWORDHASH_SIZE];
static qboolean hashSetup = qfalse;

static void WP_SaberSetupKeywordHash( void ) {
	int i;

	memset( saberParseKeywordHash, 0, sizeof( saberParseKeywordHash ) );
	for ( i=0; saberParseKeywords[i].keyword; i++ )
		KeywordHash_Add( saberParseKeywordHash, &saberParseKeywords[i] );

	hashSetup = qtrue;
}

qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber, qboolean setColors ) 
{
	const char	*token;
	const char	*p;
	keywordHash_t *key;

	// make sure the hash table has been setup
	if ( !hashSetup )
		WP_SaberSetupKeywordHash();

	if ( !saber )
		return qfalse;
	
	//Set defaults so that, if it fails, there's at least something there
	WP_SaberSetDefaults( saber, setColors );

	if ( !VALIDSTRING( SaberName ) ) 
		return qfalse;
	
	//check if we want to set the sabercolors or not (for if we're loading a savegame)
	Saber_SetColor = setColors;

	//try to parse it out
	p = SaberParms;
	COM_ParseSession ps;

	// look for the right saber
	while ( p ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			return qfalse;

		if ( !Q_stricmp( token, SaberName ) )
			break;

		SkipBracedSection( &p );
	}

	if ( !p )
		return qfalse;

	saber->name = G_NewString( SaberName );

	if ( G_ParseLiteral( &p, "{" ) )
		return qfalse;

	// parse the saber info block
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) {
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s' (WP_SaberParseParms)\n", SaberName );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) )
			break;

		key = KeywordHash_Find( saberParseKeywordHash, token );
		if ( key ) {
			key->func( saber, &p );
			continue;
		}

		gi.Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, SaberName );
		SkipRestOfLine( &p );
	}

	//FIXME: precache the saberModel(s)?

	if ( saber->type == SABER_SITH_SWORD )
	{//precache all the sith sword sounds
		Saber_SithSwordPrecache();
	}
	return qtrue;
}

void WP_RemoveSaber( gentity_t *ent, int saberNum )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	//reset everything for this saber just in case
	WP_SaberSetDefaults( &ent->client->ps.saber[saberNum] );

	ent->client->ps.dualSabers = qfalse;
	ent->client->ps.saber[saberNum].Deactivate();
	ent->client->ps.saber[saberNum].SetLength( 0.0f );
	if ( ent->weaponModel[saberNum] > 0 )
	{
		gi.G2API_SetSkin( &ent->ghoul2[ent->weaponModel[saberNum]], -1, 0 );
		gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[saberNum] );
		ent->weaponModel[saberNum] = -1;
	}
	if ( ent->client->ps.saberAnimLevel == SS_DUAL
		|| ent->client->ps.saberAnimLevel == SS_STAFF )
	{//change to the style to the default
		for ( int i = SS_NONE+1; i < SS_NUM_SABER_STYLES; i++ )
		{
			if ( (ent->client->ps.saberStylesKnown&(1<<i)) )
			{
				ent->client->ps.saberAnimLevel = i;
				if ( ent->s.number < MAX_CLIENTS )
				{
					cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel; 
				}
				break;
			}
		}
	}
}

void WP_SetSaber( gentity_t *ent, int saberNum, const char *saberName )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	if ( Q_stricmp( "none", saberName ) == 0 || Q_stricmp( "remove", saberName ) == 0 )
	{
		WP_RemoveSaber( ent, saberNum );
		return;
	}
	if ( ent->weaponModel[saberNum] > 0 )
	{
		gi.G2API_RemoveGhoul2Model(ent->ghoul2, ent->weaponModel[saberNum]);
		ent->weaponModel[saberNum] = -1;
	}
	WP_SaberParseParms( saberName, &ent->client->ps.saber[saberNum] );//get saber info
	if ( ent->client->ps.saber[saberNum].stylesLearned )
	{
		ent->client->ps.saberStylesKnown |= ent->client->ps.saber[saberNum].stylesLearned;
	}
	if ( ent->client->ps.saber[saberNum].singleBladeStyle )
	{
		ent->client->ps.saberStylesKnown |= ent->client->ps.saber[saberNum].singleBladeStyle;
	}
	if ( saberNum == 1 && (ent->client->ps.saber[1].saberFlags&SFL_TWO_HANDED) )
	{//not allowed to use a 2-handed saber as second saber
		WP_RemoveSaber( ent, saberNum );
		return;
	}
	G_ModelIndex( ent->client->ps.saber[saberNum].model );
	WP_SaberInitBladeData( ent );
	if ( saberNum == 1 )
	{//now have 2 sabers
		ent->client->ps.dualSabers = qtrue;
	}
	/*
	else if ( saberNum == 0 )
	{
		if ( ent->weaponModel[1] == -1 )
		{//don't have 2 sabers
			ent->client->ps.dualSabers = qfalse;
		}
	}
	*/
	WP_SaberAddG2SaberModels( ent, saberNum );
	ent->client->ps.saber[saberNum].SetLength( 0.0f );
	ent->client->ps.saber[saberNum].Activate();

	if ( ent->client->ps.saber[saberNum].stylesLearned )
	{//change to the style we're supposed to be using
		ent->client->ps.saberStylesKnown |= ent->client->ps.saber[saberNum].stylesLearned;
	}
	if ( ent->client->ps.saber[saberNum].singleBladeStyle )
	{
		ent->client->ps.saberStylesKnown |= ent->client->ps.saber[saberNum].singleBladeStyle;
	}
	WP_UseFirstValidSaberStyle( ent, &ent->client->ps.saberAnimLevel );
	if ( ent->s.number < MAX_CLIENTS )
	{
		cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel; 
	}
}

void WP_SaberSetColor( gentity_t *ent, int saberNum, int bladeNum, char *colorName )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	ent->client->ps.saber[saberNum].blade[bladeNum].color = TranslateSaberColor( colorName );
}

extern void WP_SetSaberEntModelSkin( gentity_t *ent, gentity_t *saberent );
qboolean WP_BreakSaber( gentity_t *ent, const char *surfName, saberType_t saberType )
{//Make sure there *is* one specified and not using dualSabers
	if ( ent == NULL || ent->client == NULL )
	{//invalid ent or client
		return qfalse;
	}

	if ( ent->s.number < MAX_CLIENTS )
	{//player
		//if ( g_spskill->integer < 3 )
		{//only on the hardest level?
			//FIXME: add a cvar?
			return qfalse;
		}
	}

	if ( ent->health <= 0 )
	{//not if they're dead
		return qfalse;
	}

	if ( ent->client->ps.weapon != WP_SABER )
	{//not holding saber
		return qfalse;
	}

	if ( ent->client->ps.dualSabers )
	{//FIXME: handle this?
		return qfalse;
	}

	if ( !ent->client->ps.saber[0].brokenSaber1 ) 
	{//not breakable into another type of saber
		return qfalse;
	}

	if ( PM_SaberInStart( ent->client->ps.saberMove ) //in a start
		|| PM_SaberInTransition( ent->client->ps.saberMove ) //in a transition
		|| PM_SaberInAttack( ent->client->ps.saberMove ) )//in an attack
	{//don't break when in the middle of an attack
		return qfalse;
	}

	if ( Q_stricmpn( "w_", surfName, 2 ) 
		&& Q_stricmpn( "saber", surfName, 5 ) //hack because using mod-community made saber
		&& Q_stricmp( "cylinder01", surfName ) )//hack because using mod-community made saber
	{//didn't hit my weapon
		return qfalse;
	}

	//Sith Sword should ALWAYS do this
	if ( saberType != SABER_SITH_SWORD && Q_irand( 0, 50 ) )//&& Q_irand( 0, 10 ) )
	{//10% chance - FIXME: extern this, too?
		return qfalse;
	}

	//break it
	char	*replacementSaber1 = G_NewString( ent->client->ps.saber[0].brokenSaber1 );
	char	*replacementSaber2 = G_NewString( ent->client->ps.saber[0].brokenSaber2 );
	int		i, originalNumBlades = ent->client->ps.saber[0].numBlades;
	qboolean	broken = qfalse;
	saber_colors_t	colors[MAX_BLADES];

	//store the colors
	for ( i = 0; i < MAX_BLADES; i++ )
	{
		colors[i] = ent->client->ps.saber[0].blade[i].color;
	}

	//FIXME: chance of dropping the right-hand one?  Based on damage, or...?
	//FIXME: sound & effect when this happens, and send them into a broken parry?

	//remove saber[0], replace with replacementSaber1
	if ( replacementSaber1 )
	{
		WP_RemoveSaber( ent, 0 );
		WP_SetSaber( ent, 0, replacementSaber1 );
		for ( i = 0; i < ent->client->ps.saber[0].numBlades; i++ )
		{
			ent->client->ps.saber[0].blade[i].color = colors[i];
		}
		broken = qtrue;
		//change my saberent's model and skin to match my new right-hand saber
		WP_SetSaberEntModelSkin( ent, &g_entities[ent->client->ps.saberEntityNum] );
	}

	if ( originalNumBlades <= 1 ) 
	{//nothing to split off
		//FIXME: handle this?
	}
	else
	{
		//remove saber[1], replace with replacementSaber2
		if ( replacementSaber2 )
		{//FIXME: 25% chance that it just breaks - just spawn the second saber piece and toss it away immediately, can't be picked up.
			//shouldn't be one in this hand, but just in case, remove it
			WP_RemoveSaber( ent, 1 );
			WP_SetSaber( ent, 1, replacementSaber2 );

			//put the remainder of the original saber's blade colors onto this saber's blade(s)
			for ( i = ent->client->ps.saber[0].numBlades; i < MAX_BLADES; i++ )
			{
				ent->client->ps.saber[1].blade[i-ent->client->ps.saber[0].numBlades].color = colors[i];
			}
			broken = qtrue;
		}
	}
	return broken;
}

void WP_SaberLoadParms( void ) 
{
	int			len, totallen, saberExtFNLen, fileCnt, i;
	char		*buffer, *holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in

	//gi.Printf( "Parsing *.sab saber definitions\n" );

	//set where to store the first one
	totallen = 0;
	marker = SaberParms;
	marker[0] = '\0';

	//now load in the .sab definitions
	fileCnt = gi.FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) 
	{
		saberExtFNLen = strlen( holdChar );

		len = gi.FS_ReadFile( va( "ext_data/sabers/%s", holdChar), (void **) &buffer );

		if ( len == -1 ) 
		{
			gi.Printf( "WP_SaberLoadParms: error reading %s\n", holdChar );
		}
		else
		{
			if ( totallen && *(marker-1) == '}' )
			{//don't let it end on a } because that should be a stand-alone token
				strcat( marker, " " );
				totallen++;
				marker++; 
			}
			len = COM_Compress( buffer );

			if ( totallen + len >= MAX_SABER_DATA_SIZE ) {
				G_Error( "WP_SaberLoadParms: ran out of space before reading %s\n(you must make the .sab files smaller)", holdChar  );
			}
			strcat( marker, buffer );
			gi.FS_FreeFile( buffer );

			totallen += len;
			marker += len;
		}
	}
}
