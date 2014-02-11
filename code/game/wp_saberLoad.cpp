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

#define MAX_SABER_DATA_SIZE 0x80000
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


saber_styles_t TranslateSaberStyle( const char *name ) 
{
	if ( !Q_stricmp( name, "fast" ) ) 
	{
		return SS_FAST;
	}
	if ( !Q_stricmp( name, "medium" ) ) 
	{
		return SS_MEDIUM;
	}
	if ( !Q_stricmp( name, "strong" ) ) 
	{
		return SS_STRONG;
	}
	if ( !Q_stricmp( name, "desann" ) ) 
	{
		return SS_DESANN;
	}
	if ( !Q_stricmp( name, "tavion" ) ) 
	{
		return SS_TAVION;
	}
	if ( !Q_stricmp( name, "dual" ) ) 
	{
		return SS_DUAL;
	}
	if ( !Q_stricmp( name, "staff" ) ) 
	{
		return SS_STAFF;
	}
	return SS_NONE;
}

void WP_SaberFreeStrings( saberInfo_t &saber )
{
	if (saber.name && gi.bIsFromZone(saber.name , TAG_G_ALLOC)) {
		gi.Free (saber.name);
		saber.name=0;
	}
	if (saber.fullName && gi.bIsFromZone(saber.fullName , TAG_G_ALLOC)) {
		gi.Free (saber.fullName);
		saber.fullName=0;
	}
	if (saber.model && gi.bIsFromZone(saber.model , TAG_G_ALLOC)) {
		gi.Free (saber.model);
		saber.model=0;
	}
	if (saber.skin && gi.bIsFromZone(saber.skin , TAG_G_ALLOC)) {
		gi.Free (saber.skin);
		saber.skin=0;
	}
	if (saber.brokenSaber1 && gi.bIsFromZone(saber.brokenSaber1 , TAG_G_ALLOC)) {
		gi.Free (saber.brokenSaber1);
		saber.brokenSaber1=0;
	}
	if (saber.brokenSaber2 && gi.bIsFromZone(saber.brokenSaber2 , TAG_G_ALLOC)) {
		gi.Free (saber.brokenSaber2);
		saber.brokenSaber2=0;
	}
}

qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum )
{
	if ( saber )
	{
		if ( saber->bladeStyle2Start > 0 )
		{
			if ( bladeNum >= saber->bladeStyle2Start )
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean WP_SaberBladeDoTransitionDamage( saberInfo_t *saber, int bladeNum )
{
	if ( !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
		&& (saber->saberFlags2&SFL2_TRANSITION_DAMAGE) )
	{//use first blade style for this blade
		return qtrue;
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
		&& (saber->saberFlags2&SFL2_TRANSITION_DAMAGE2) )
	{//use second blade style for this blade
		return qtrue;
	}
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

qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber, qboolean setColors ) 
{
	const char	*token;
	const char	*value;
	const char	*p;
	float	f;
	int		n;

	if ( !saber ) 
	{
		return qfalse;
	}
	
	//Set defaults so that, if it fails, there's at least something there
	WP_SaberSetDefaults( saber, setColors );

	if ( !SaberName || !SaberName[0] ) 
	{
		return qfalse;
	}

	saber->name = G_NewString( SaberName );
	//try to parse it out
	p = SaberParms;
	COM_BeginParseSession();

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			COM_EndParseSession(  );
			return qfalse;
		}

		if ( !Q_stricmp( token, SaberName ) ) 
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p ) 
	{
		COM_EndParseSession(  );
		return qfalse;
	}

	if ( G_ParseLiteral( &p, "{" ) ) 
	{
		COM_EndParseSession(  );
		return qfalse;
	}
		
	// parse the saber info block
	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", SaberName );
			COM_EndParseSession(  );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) ) 
		{
			break;
		}

#ifdef _WIN32
#pragma region(Saber Parms)
#endif

		//saber fullName
		if ( !Q_stricmp( token, "name" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->fullName = G_NewString( value );
			continue;
		}

		//saber type
		if ( !Q_stricmp( token, "saberType" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberType = GetIDForString( SaberTable, value );
			if ( saberType >= SABER_SINGLE && saberType <= NUM_SABERS )
			{
				saber->type = (saberType_t)saberType;
			}
			continue;
		}

		//saber hilt
		if ( !Q_stricmp( token, "saberModel" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->model = G_NewString( value );
			continue;
		}

		if ( !Q_stricmp( token, "customSkin" ) )
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->skin = G_NewString( value );
			continue;
		}

		//on sound
		if ( !Q_stricmp( token, "soundOn" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOn = G_SoundIndex( G_NewString( value ) );
			continue;
		}

		//loop sound
		if ( !Q_stricmp( token, "soundLoop" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundLoop = G_SoundIndex( G_NewString( value ) );
			continue;
		}

		//off sound
		if ( !Q_stricmp( token, "soundOff" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOff = G_SoundIndex( G_NewString( value ) );
			continue;
		}

		if ( !Q_stricmp( token, "numBlades" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n < 1 || n > MAX_BLADES )
			{
				G_Error( "WP_SaberParseParms: saber %s has illegal number of blades (%d) max: %d", SaberName, n, MAX_BLADES );
				continue;
			}
			saber->numBlades = n;
			continue;
		}

		// saberColor
		if ( !Q_stricmpn( token, "saberColor", 10 ) ) 
		{
			if ( !setColors )
			{//don't actually want to set the colors
				//read the color out anyway just to advance the *p pointer
				COM_ParseString( &p, &value );
				continue;
			}
			else
			{
				if (strlen(token)==10)
				{
					n = -1;
				}
				else if (strlen(token)==11)
				{
					n = atoi(&token[10])-1;
					if (n > 7 || n < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, SaberName );
						//read the color out anyway just to advance the *p pointer
						COM_ParseString( &p, &value );
						continue;
					}
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, SaberName );
					//read the color out anyway just to advance the *p pointer
					COM_ParseString( &p, &value );
					continue;
				}

				if ( COM_ParseString( &p, &value ) )	//read the color
				{
					continue;
				}

				if (n==-1)
				{//NOTE: this fills in the rest of the blades with the same color by default
					saber_colors_t color = TranslateSaberColor( value );
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						saber->blade[n].color = color;
					}
				} else 
				{
					saber->blade[n].color = TranslateSaberColor( value );
				}
				continue;
			}
		}

		//saber length
		if ( !Q_stricmpn( token, "saberLength", 11 ) ) 
		{
			if (strlen(token)==11)
			{
				n = -1;
			}
			else if (strlen(token)==12)
			{
				n = atoi(&token[11])-1;
				if (n > 7 || n < 1 )
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, SaberName );
					continue;
				}
			}
			else
			{
				gi.Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, SaberName );
				continue;
			}

			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			//cap
			if ( f < 4.0f )
			{
				f = 4.0f;
			}

			if (n==-1)
			{//NOTE: this fills in the rest of the blades with the same length by default
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					saber->blade[n].lengthMax = f;
				}
			}
			else
			{
				saber->blade[n].lengthMax = f;
			}
			continue;
		}

		//blade radius
		if ( !Q_stricmpn( token, "saberRadius", 11 ) ) 
		{
			if (strlen(token)==11)
			{
				n = -1;
			}
			else if (strlen(token)==12)
			{
				n = atoi(&token[11])-1;
				if (n > 7 || n < 1 )
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, SaberName );
					continue;
				}
			}
			else
			{
				gi.Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, SaberName );
				continue;
			}

			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			//cap
			if ( f < 0.25f )
			{
				f = 0.25f;
			}
			if (n==-1)
			{//NOTE: this fills in the rest of the blades with the same length by default
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					saber->blade[n].radius = f;
				}
			}
			else
			{
				saber->blade[n].radius = f;
			}
			continue;
		}

		//locked saber style
		if ( !Q_stricmp( token, "saberStyle" ) ) 
		{
			int style, styleNum;
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			//OLD WAY: only allowed ONE style
			style = TranslateSaberStyle( value );
			//learn only this style
			saber->stylesLearned = (1<<style);
			//forbid all other styles
			saber->stylesForbidden = 0;
			for ( styleNum = SS_NONE+1; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
			{
				if ( styleNum != style )
				{
					saber->stylesForbidden |= (1<<styleNum);
				}
			}
			continue;
		}

		//learned saber style
		if ( !Q_stricmp( token, "saberStyleLearned" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->stylesLearned |= (1<<TranslateSaberStyle( value ));
			continue;
		}

		//forbidden saber style
		if ( !Q_stricmp( token, "saberStyleForbidden" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->stylesForbidden |= (1<<TranslateSaberStyle( value ));
			continue;
		}

		//maxChain
		if ( !Q_stricmp( token, "maxChain" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->maxChain = n;
			continue;
		}

		//lockable
		if ( !Q_stricmp( token, "lockable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n == 0 )
			{
				saber->saberFlags |= SFL_NOT_LOCKABLE;
			}
			continue;
		}

		//throwable
		if ( !Q_stricmp( token, "throwable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n == 0 )
			{
				saber->saberFlags |= SFL_NOT_THROWABLE;
			}
			continue;
		}

		//disarmable
		if ( !Q_stricmp( token, "disarmable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n == 0 )
			{
				saber->saberFlags |= SFL_NOT_DISARMABLE;
			}
			continue;
		}

		//active blocking
		if ( !Q_stricmp( token, "blocking" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n == 0 )
			{
				saber->saberFlags |= SFL_NOT_ACTIVE_BLOCKING;
			}
			continue;
		}

		//twoHanded
		if ( !Q_stricmp( token, "twoHanded" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_TWO_HANDED;
			}
			continue;
		}

		//force power restrictions
		if ( !Q_stricmp( token, "forceRestrict" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int fp = GetIDForString( FPTable, value );
			if ( fp >= FP_FIRST && fp < NUM_FORCE_POWERS )
			{
				saber->forceRestrictions |= (1<<fp);
			}
			continue;
		}

		//lockBonus
		if ( !Q_stricmp( token, "lockBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->lockBonus = n;
			continue;
		}

		//parryBonus
		if ( !Q_stricmp( token, "parryBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->parryBonus = n;
			continue;
		}

		//breakParryBonus
		if ( !Q_stricmp( token, "breakParryBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->breakParryBonus = n;
			continue;
		}

		//breakParryBonus2
		if ( !Q_stricmp( token, "breakParryBonus2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->breakParryBonus2 = n;
			continue;
		}

		//disarmBonus
		if ( !Q_stricmp( token, "disarmBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->disarmBonus = n;
			continue;
		}

		//disarmBonus2
		if ( !Q_stricmp( token, "disarmBonus2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->disarmBonus2 = n;
			continue;
		}

		//single blade saber style
		if ( !Q_stricmp( token, "singleBladeStyle" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->singleBladeStyle = TranslateSaberStyle( value );
			continue;
		}

		//single blade throwable
		if ( !Q_stricmp( token, "singleBladeThrowable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_SINGLE_BLADE_THROWABLE;
			}
			continue;
		}

		//broken replacement saber1 (right hand)
		if ( !Q_stricmp( token, "brokenSaber1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->brokenSaber1 = G_NewString( value );
			continue;
		}
		
		//broken replacement saber2 (left hand)
		if ( !Q_stricmp( token, "brokenSaber2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->brokenSaber2 = G_NewString( value );
			continue;
		}

		//spins and does damage on return from saberthrow
		if ( !Q_stricmp( token, "returnDamage" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_RETURN_DAMAGE;
			}
			continue;
		}

		//spin sound (when thrown)
		if ( !Q_stricmp( token, "spinSound" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->spinSound = G_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//fall sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "fallSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->fallSound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//fall sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "fallSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->fallSound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//fall sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "fallSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->fallSound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//you move faster/slower when using this saber
		if ( !Q_stricmp( token, "moveSpeedScale" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->moveSpeedScale = f;
			continue;
		}

		//plays normal attack animations faster/slower
		if ( !Q_stricmp( token, "animSpeedScale" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->animSpeedScale = f;
			continue;
		}

		//if set, weapon stays active even in water
		if ( !Q_stricmp( token, "onInWater" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_ON_IN_WATER;
			}
			continue;
		}

		//if non-zero, the saber will bounce back when it hits solid architecture (good for real-sword type mods)
		if ( !Q_stricmp( token, "bounceOnWalls" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_BOUNCE_ON_WALLS;
			}
			continue;
		}

		//if set, saber model is bolted to wrist, not in hand... useful for things like claws & shields, etc.
		if ( !Q_stricmp( token, "boltToWrist" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_BOLT_TO_WRIST;
			}
			continue;
		}

		//kata move
		if ( !Q_stricmp( token, "kataMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->kataMove = saberMove;				//LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time 
			}
			continue;
		}
		//lungeAtkMove move
		if ( !Q_stricmp( token, "lungeAtkMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->lungeAtkMove = saberMove;
			}
			continue;
		}
		//jumpAtkUpMove move
		if ( !Q_stricmp( token, "jumpAtkUpMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->jumpAtkUpMove = saberMove;
			}
			continue;
		}
		//jumpAtkFwdMove move
		if ( !Q_stricmp( token, "jumpAtkFwdMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->jumpAtkFwdMove = saberMove;
			}
			continue;
		}
		//jumpAtkBackMove move
		if ( !Q_stricmp( token, "jumpAtkBackMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->jumpAtkBackMove = saberMove;
			}
			continue;
		}
		//jumpAtkRightMove move
		if ( !Q_stricmp( token, "jumpAtkRightMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->jumpAtkRightMove = saberMove;
			}
			continue;
		}
		//jumpAtkLeftMove move
		if ( !Q_stricmp( token, "jumpAtkLeftMove" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberMove = GetIDForString( SaberMoveTable, value );
			if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
			{
				saber->jumpAtkLeftMove = saberMove;
			}
			continue;
		}
		//readyAnim
		if ( !Q_stricmp( token, "readyAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->readyAnim = anim;
			}
			continue;
		}
		//drawAnim
		if ( !Q_stricmp( token, "drawAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->drawAnim = anim;
			}
			continue;
		}
		//putawayAnim
		if ( !Q_stricmp( token, "putawayAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->putawayAnim = anim;
			}
			continue;
		}
		//tauntAnim
		if ( !Q_stricmp( token, "tauntAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->tauntAnim = anim;
			}
			continue;
		}
		//bowAnim
		if ( !Q_stricmp( token, "bowAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->bowAnim = anim;
			}
			continue;
		}
		//meditateAnim
		if ( !Q_stricmp( token, "meditateAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->meditateAnim = anim;
			}
			continue;
		}
		//flourishAnim
		if ( !Q_stricmp( token, "flourishAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->flourishAnim = anim;
			}
			continue;
		}
		//gloatAnim
		if ( !Q_stricmp( token, "gloatAnim" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int anim = GetIDForString( animTable, value );
			if ( anim >= 0 && anim < MAX_ANIMATIONS )
			{
				saber->gloatAnim = anim;
			}
			continue;
		}

		//if set, cannot do roll-stab move at end of roll
		if ( !Q_stricmp( token, "noRollStab" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_ROLL_STAB;
			}
			continue;
		}

		//if set, cannot do pull+attack move (move not available in MP anyway)
		if ( !Q_stricmp( token, "noPullAttack" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_PULL_ATTACK;
			}
			continue;
		}

		//if set, cannot do back-stab moves
		if ( !Q_stricmp( token, "noBackAttack" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_BACK_ATTACK;
			}
			continue;
		}

		//if set, cannot do stabdown move (when enemy is on ground)
		if ( !Q_stricmp( token, "noStabDown" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_STABDOWN;
			}
			continue;
		}

		//if set, cannot side-run or forward-run on walls
		if ( !Q_stricmp( token, "noWallRuns" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_WALL_RUNS;
			}
			continue;
		}

		//if set, cannot do backflip off wall or side-flips off walls
		if ( !Q_stricmp( token, "noWallFlips" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_WALL_FLIPS;
			}
			continue;
		}

		//if set, cannot grab wall & jump off
		if ( !Q_stricmp( token, "noWallGrab" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_WALL_GRAB;
			}
			continue;
		}

		//if set, cannot roll
		if ( !Q_stricmp( token, "noRolls" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_ROLLS;
			}
			continue;
		}

		//if set, cannot do flips
		if ( !Q_stricmp( token, "noFlips" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_FLIPS;
			}
			continue;
		}

		//if set, cannot do cartwheels
		if ( !Q_stricmp( token, "noCartwheels" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_CARTWHEELS;
			}
			continue;
		}

		//if set, cannot do kicks (can't do kicks anyway if using a throwable saber/sword)
		if ( !Q_stricmp( token, "noKicks" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_KICKS;
			}
			continue;
		}

		//if set, cannot do the simultaneous attack left/right moves (only available in Dual Lightsaber Combat Style)
		if ( !Q_stricmp( token, "noMirrorAttacks" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags |= SFL_NO_MIRROR_ATTACKS;
			}
			continue;
		}

		if ( !Q_stricmp( token, "notInMP" ) ) 
		{//ignore this
			SkipRestOfLine( &p );
			continue;
		}

//===ABOVE THIS, ALL VALUES ARE GLOBAL TO THE SABER========================================================
		//bladeStyle2Start - where to start using the second set of blade data
		if ( !Q_stricmp( token, "bladeStyle2Start" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->bladeStyle2Start = n;
			continue;
		}
//===BLADE-SPECIFIC FIELDS=================================================================================

		//===PRIMARY BLADE====================================
		//stops the saber from drawing marks on the world (good for real-sword type mods)
		if ( !Q_stricmp( token, "noWallMarks" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_WALL_MARKS;
			}
			continue;
		}

		//stops the saber from drawing a dynamic light (good for real-sword type mods)
		if ( !Q_stricmp( token, "noDlight" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_DLIGHT;
			}
			continue;
		}

		//stops the saber from drawing a blade (good for real-sword type mods)
		if ( !Q_stricmp( token, "noBlade" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_BLADE;
			}
			continue;
		}

		//default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
		if ( !Q_stricmp( token, "trailStyle" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->trailStyle = n;
			continue;
		}

		//if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
		if ( !Q_stricmp( token, "g2MarksShader" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			Q_strncpyz( saber->g2MarksShader, value, sizeof(saber->g2MarksShader), qtrue );
			//NOTE: registers this on cgame side where it registers all client assets
			continue;
		}
		
		//if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
		if ( !Q_stricmp( token, "g2WeaponMarkShader" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			Q_strncpyz( saber->g2WeaponMarkShader, value, sizeof(saber->g2WeaponMarkShader), qtrue );
			//NOTE: registers this on cgame side where it registers all client assets
			continue;
		}

		//if non-zero, uses damage done to calculate an appropriate amount of knockback
		if ( !Q_stricmp( token, "knockbackScale" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->knockbackScale = f;
			continue;
		}

		//scale up or down the damage done by the saber
		if ( !Q_stricmp( token, "damageScale" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->damageScale = f;
			continue;
		}

		//if non-zero, the saber never does dismemberment (good for pointed/blunt melee weapons)
		if ( !Q_stricmp( token, "noDismemberment" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_DISMEMBERMENT;
			}
			continue;
		}

		//if non-zero, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
		if ( !Q_stricmp( token, "noIdleEffect" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_IDLE_EFFECT;
			}
			continue;
		}

		//if set, the blades will always be blocking (good for things like shields that should always block)
		if ( !Q_stricmp( token, "alwaysBlock" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_ALWAYS_BLOCK;
			}
			continue;
		}

		//if set, the blades cannot manually be toggled on and off
		if ( !Q_stricmp( token, "noManualDeactivate" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_MANUAL_DEACTIVATE;
			}
			continue;
		}

		//if set, the blade does damage in start, transition and return anims (like strong style does)
		if ( !Q_stricmp( token, "transitionDamage" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_TRANSITION_DAMAGE;
			}
			continue;
		}

		//splashRadius - radius of splashDamage
		if ( !Q_stricmp( token, "splashRadius" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->splashRadius = f;
			continue;
		}
		
		//splashDamage - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
		if ( !Q_stricmp( token, "splashDamage" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->splashDamage = n;
			continue;
		}

		//splashKnockback - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
		if ( !Q_stricmp( token, "splashKnockback" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->splashKnockback = f;
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounceSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounceSound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounceSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounceSound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounceSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounceSound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//block effect - when saber/sword hits another saber/sword
		if ( !Q_stricmp( token, "blockEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockEffect = G_EffectIndex( (char *)value );
			continue;
		}

		//hit person effect - when saber/sword hits a person
		if ( !Q_stricmp( token, "hitPersonEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitPersonEffect = G_EffectIndex( (char *)value );
			continue;
		}

		//hit other effect - when saber/sword hits sopmething else damagable
		if ( !Q_stricmp( token, "hitOtherEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitOtherEffect = G_EffectIndex( (char *)value );
			continue;
		}

		//blade effect
		if ( !Q_stricmp( token, "bladeEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bladeEffect = G_EffectIndex( (char *)value );
			continue;
		}

		//if non-zero, the saber will not do the big, white clash flare with other sabers
		if ( !Q_stricmp( token, "noClashFlare" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_CLASH_FLARE;
			}
			continue;
		}

		//===SECONDARY BLADE====================================
		//stops the saber from drawing marks on the world (good for real-sword type mods)
		if ( !Q_stricmp( token, "noWallMarks2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_WALL_MARKS2;
			}
			continue;
		}

		//stops the saber from drawing a dynamic light (good for real-sword type mods)
		if ( !Q_stricmp( token, "noDlight2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_DLIGHT2;
			}
			continue;
		}

		//stops the saber from drawing a blade (good for real-sword type mods)
		if ( !Q_stricmp( token, "noBlade2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_BLADE2;
			}
			continue;
		}

		//default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
		if ( !Q_stricmp( token, "trailStyle2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->trailStyle2 = n;
			continue;
		}

		//if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
		if ( !Q_stricmp( token, "g2MarksShader2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			Q_strncpyz( saber->g2MarksShader2, value, sizeof(saber->g2MarksShader2), qtrue );
			//NOTE: registers this on cgame side where it registers all client assets
			continue;
		}

		//if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
		if ( !Q_stricmp( token, "g2WeaponMarkShader2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			Q_strncpyz( saber->g2WeaponMarkShader2, value, sizeof(saber->g2WeaponMarkShader2), qtrue );
			//NOTE: registers this on cgame side where it registers all client assets
			continue;
		}

		//if non-zero, uses damage done to calculate an appropriate amount of knockback
		if ( !Q_stricmp( token, "knockbackScale2" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->knockbackScale2 = f;
			continue;
		}

		//scale up or down the damage done by the saber
		if ( !Q_stricmp( token, "damageScale2" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->damageScale2 = f;
			continue;
		}

		//if non-zero, the saber never does dismemberment (good for pointed/blunt melee weapons)
		if ( !Q_stricmp( token, "noDismemberment2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_DISMEMBERMENT2;
			}
			continue;
		}

		//if non-zero, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
		if ( !Q_stricmp( token, "noIdleEffect2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_IDLE_EFFECT2;
			}
			continue;
		}

		//if set, the blades will always be blocking (good for things like shields that should always block)
		if ( !Q_stricmp( token, "alwaysBlock2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_ALWAYS_BLOCK2;
			}
			continue;
		}

		//if set, the blades cannot manually be toggled on and off
		if ( !Q_stricmp( token, "noManualDeactivate2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_MANUAL_DEACTIVATE2;
			}
			continue;
		}

		//if set, the blade does damage in start, transition and return anims (like strong style does)
		if ( !Q_stricmp( token, "transitionDamage2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_TRANSITION_DAMAGE2;
			}
			continue;
		}

		//splashRadius - radius of splashDamage
		if ( !Q_stricmp( token, "splashRadius2" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->splashRadius2 = f;
			continue;
		}
		
		//splashDamage - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
		if ( !Q_stricmp( token, "splashDamage2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->splashDamage2 = n;
			continue;
		}

		//splashKnockback - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
		if ( !Q_stricmp( token, "splashKnockback2" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->splashKnockback2 = f;
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hit2Sound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hit2Sound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hit2Sound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hit2Sound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hit2Sound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hit2Sound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "block2Sound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->block2Sound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "block2Sound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->block2Sound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "block2Sound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->block2Sound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounce2Sound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounce2Sound[0] = G_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounce2Sound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounce2Sound[1] = G_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounce2Sound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounce2Sound[2] = G_SoundIndex( (char *)value );
			continue;
		}

		//block effect - when saber/sword hits another saber/sword
		if ( !Q_stricmp( token, "blockEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockEffect2 = G_EffectIndex( (char *)value );
			continue;
		}

		//hit person effect - when saber/sword hits a person
		if ( !Q_stricmp( token, "hitPersonEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitPersonEffect2 = G_EffectIndex( (char *)value );
			continue;
		}

		//hit other effect - when saber/sword hits sopmething else damagable
		if ( !Q_stricmp( token, "hitOtherEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitOtherEffect2 = G_EffectIndex( (char *)value );
			continue;
		}

		//blade effect 2
		if ( !Q_stricmp( token, "bladeEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bladeEffect = G_EffectIndex( (char *)value );
			continue;
		}

		//if non-zero, the saber will not do the big, white clash flare with other sabers
		if ( !Q_stricmp( token, "noClashFlare2" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n )
			{
				saber->saberFlags2 |= SFL2_NO_CLASH_FLARE2;
			}
			continue;
		}

//===END BLADE-SPECIFIC FIELDS=============================================================================

		gi.Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, SaberName );
		SkipRestOfLine( &p );
	}

	//FIXME: precache the saberModel(s)?

	if ( saber->type == SABER_SITH_SWORD )
	{//precache all the sith sword sounds
		Saber_SithSwordPrecache();
	}
#ifdef _WIN32
#pragma endregion
#endif
	COM_EndParseSession(  );
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
