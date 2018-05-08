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

// bg_saberLoad.c
// game and cgame, NOT ui

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "w_saber.h"

#ifdef _GAME
	#include "g_local.h"
#elif _CGAME
	#include "cgame/cg_local.h"
#elif UI_BUILD
	#include "ui/ui_local.h"
#endif

extern stringID_table_t animTable[MAX_ANIMATIONS+1];

int BG_SoundIndex( const char *sound ) {
#ifdef _GAME
	return G_SoundIndex( sound );
#elif defined(_CGAME) || defined(UI_BUILD)
	return trap->S_RegisterSound( sound );
#endif
}

extern stringID_table_t FPTable[];

#define MAX_SABER_DATA_SIZE (1024*1024) // 1mb, was 512kb
static char saberParms[MAX_SABER_DATA_SIZE];

stringID_table_t saberTable[] = {
	ENUM2STRING( SABER_NONE ),
	ENUM2STRING( SABER_SINGLE ),
	ENUM2STRING( SABER_STAFF ),
	ENUM2STRING( SABER_BROAD ),
	ENUM2STRING( SABER_PRONG ),
	ENUM2STRING( SABER_DAGGER ),
	ENUM2STRING( SABER_ARC ),
	ENUM2STRING( SABER_SAI ),
	ENUM2STRING( SABER_CLAW ),
	ENUM2STRING( SABER_LANCE ),
	ENUM2STRING( SABER_STAR ),
	ENUM2STRING( SABER_TRIDENT ),
	{ "", -1 }
};

stringID_table_t saberMoveTable[] = {
	ENUM2STRING( LS_NONE ),
	// Attacks
	ENUM2STRING( LS_A_TL2BR ),
	ENUM2STRING( LS_A_L2R ),
	ENUM2STRING( LS_A_BL2TR ),
	ENUM2STRING( LS_A_BR2TL ),
	ENUM2STRING( LS_A_R2L ),
	ENUM2STRING( LS_A_TR2BL ),
	ENUM2STRING( LS_A_T2B ),
	ENUM2STRING( LS_A_BACKSTAB ),
	ENUM2STRING( LS_A_BACK ),
	ENUM2STRING( LS_A_BACK_CR ),
	ENUM2STRING( LS_ROLL_STAB ),
	ENUM2STRING( LS_A_LUNGE ),
	ENUM2STRING( LS_A_JUMP_T__B_ ),
	ENUM2STRING( LS_A_FLIP_STAB ),
	ENUM2STRING( LS_A_FLIP_SLASH ),
	ENUM2STRING( LS_JUMPATTACK_DUAL ),
	ENUM2STRING( LS_JUMPATTACK_ARIAL_LEFT ),
	ENUM2STRING( LS_JUMPATTACK_ARIAL_RIGHT ),
	ENUM2STRING( LS_JUMPATTACK_CART_LEFT ),
	ENUM2STRING( LS_JUMPATTACK_CART_RIGHT ),
	ENUM2STRING( LS_JUMPATTACK_STAFF_LEFT ),
	ENUM2STRING( LS_JUMPATTACK_STAFF_RIGHT ),
	ENUM2STRING( LS_BUTTERFLY_LEFT ),
	ENUM2STRING( LS_BUTTERFLY_RIGHT ),
	ENUM2STRING( LS_A_BACKFLIP_ATK ),
	ENUM2STRING( LS_SPINATTACK_DUAL ),
	ENUM2STRING( LS_SPINATTACK ),
	ENUM2STRING( LS_LEAP_ATTACK ),
	ENUM2STRING( LS_SWOOP_ATTACK_RIGHT ),
	ENUM2STRING( LS_SWOOP_ATTACK_LEFT ),
	ENUM2STRING( LS_TAUNTAUN_ATTACK_RIGHT ),
	ENUM2STRING( LS_TAUNTAUN_ATTACK_LEFT ),
	ENUM2STRING( LS_KICK_F ),
	ENUM2STRING( LS_KICK_B ),
	ENUM2STRING( LS_KICK_R ),
	ENUM2STRING( LS_KICK_L ),
	ENUM2STRING( LS_KICK_S ),
	ENUM2STRING( LS_KICK_BF ),
	ENUM2STRING( LS_KICK_RL ),
	ENUM2STRING( LS_KICK_F_AIR ),
	ENUM2STRING( LS_KICK_B_AIR ),
	ENUM2STRING( LS_KICK_R_AIR ),
	ENUM2STRING( LS_KICK_L_AIR ),
	ENUM2STRING( LS_STABDOWN ),
	ENUM2STRING( LS_STABDOWN_STAFF ),
	ENUM2STRING( LS_STABDOWN_DUAL ),
	ENUM2STRING( LS_DUAL_SPIN_PROTECT ),
	ENUM2STRING( LS_STAFF_SOULCAL ),
	ENUM2STRING( LS_A1_SPECIAL ),
	ENUM2STRING( LS_A2_SPECIAL ),
	ENUM2STRING( LS_A3_SPECIAL ),
	ENUM2STRING( LS_UPSIDE_DOWN_ATTACK ),
	ENUM2STRING( LS_PULL_ATTACK_STAB ),
	ENUM2STRING( LS_PULL_ATTACK_SWING ),
	ENUM2STRING( LS_SPINATTACK_ALORA ),
	ENUM2STRING( LS_DUAL_FB ),
	ENUM2STRING( LS_DUAL_LR ),
	ENUM2STRING( LS_HILT_BASH ),
	{ "", -1 }
};

//Also used in npc code
qboolean BG_ParseLiteral( const char **data, const char *string ) {
	const char *token;

	token = COM_ParseExt( data, qtrue );
	if ( !token[0] ) {
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) {
		Com_Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
}

qboolean BG_ParseLiteralSilent( const char **data, const char *string ) {
	const char *token;

	token = COM_ParseExt( data, qtrue );
	if ( !token[0] ) {
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) {

		return qtrue;
	}

	return qfalse;
}

saber_colors_t TranslateSaberColor( const char *name ) {
	if ( !Q_stricmp( name, "red" ) )
		return SABER_RED;
	if ( !Q_stricmp( name, "orange" ) )
		return SABER_ORANGE;
	if ( !Q_stricmp( name, "yellow" ) )
		return SABER_YELLOW;
	if ( !Q_stricmp( name, "green" ) )
		return SABER_GREEN;
	if ( !Q_stricmp( name, "blue" ) )
		return SABER_BLUE;
	if ( !Q_stricmp( name, "purple" ) )
		return SABER_PURPLE;
	if ( !Q_stricmp( name, "random" ) )
		return (saber_colors_t)Q_irand( SABER_ORANGE, SABER_PURPLE );

	return SABER_BLUE;
}

const char *SaberColorToString( saber_colors_t color ) {
	if ( color == SABER_RED )		return "red";
	if ( color == SABER_ORANGE )	return "orange";
	if ( color == SABER_YELLOW )	return "yellow";
	if ( color == SABER_GREEN )		return "green";
	if ( color == SABER_BLUE )		return "blue";
	if ( color == SABER_PURPLE )	return "purple";

	return NULL;
}

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

saberType_t TranslateSaberType( const char *name ) {
	if ( !Q_stricmp( name, "SABER_SINGLE" ) )		return SABER_SINGLE;
	if ( !Q_stricmp( name, "SABER_STAFF" ) ) 		return SABER_STAFF;
	if ( !Q_stricmp( name, "SABER_DAGGER" ) ) 		return SABER_DAGGER;
	if ( !Q_stricmp( name, "SABER_BROAD" ) ) 		return SABER_BROAD;
	if ( !Q_stricmp( name, "SABER_PRONG" ) ) 		return SABER_PRONG;
	if ( !Q_stricmp( name, "SABER_ARC" ) )			return SABER_ARC;
	if ( !Q_stricmp( name, "SABER_SAI" ) )			return SABER_SAI;
	if ( !Q_stricmp( name, "SABER_CLAW" ) )			return SABER_CLAW;
	if ( !Q_stricmp( name, "SABER_LANCE" ) )		return SABER_LANCE;
	if ( !Q_stricmp( name, "SABER_STAR" ) )			return SABER_STAR;
	if ( !Q_stricmp( name, "SABER_TRIDENT" ) )		return SABER_TRIDENT;
	if ( !Q_stricmp( name, "SABER_SITH_SWORD" ) )	return SABER_SITH_SWORD;

	return SABER_SINGLE;
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

qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel ) {
	qboolean styleInvalid = qfalse;
	qboolean saber1Active, saber2Active;
	qboolean dualSabers = qfalse;
	int	validStyles=0, styleNum;

	if ( saber2 && saber2->model[0] )
		dualSabers = qtrue;

	//dual
	if ( dualSabers ) {
		if ( saberHolstered > 1 )
			saber1Active = saber2Active = qfalse;
		else if ( saberHolstered > 0 ) {
			saber1Active = qtrue;
			saber2Active = qfalse;
		}
		else
			saber1Active = saber2Active = qtrue;
	}
	// single/staff
	else {
		saber2Active = qfalse;
		if ( !saber1 || !saber1->model[0] )
			saber1Active = qfalse;
		//staff
		else if ( saber1->numBlades > 1 ) {
			if ( saberHolstered > 1 )
				saber1Active = qfalse;
			else
				saber1Active = qtrue;
		}
		//single
		else {
			if ( saberHolstered )
				saber1Active = qfalse;
			else
				saber1Active = qtrue;
		}
	}

	//initially, all styles are valid
	validStyles = (1<<SS_NUM_SABER_STYLES)-2; // mask off 1<<SS_NONE

	// check for invalid styles
	if ( saber1Active && saber1 && saber1->model[0] && saber1->stylesForbidden ) {
		if ( (saber1->stylesForbidden & (1<<*saberAnimLevel)) ) {
			//not a valid style for first saber!
			styleInvalid = qtrue;
			validStyles &= ~saber1->stylesForbidden;
		}
	}
	if ( dualSabers ) {
		if ( saber2Active && saber2->stylesForbidden ) {
			if ( (saber2->stylesForbidden & (1<<*saberAnimLevel)) ) {
				//not a valid style for second saber!
				styleInvalid = qtrue;
				//only the ones both sabers allow is valid
				validStyles &= ~saber2->stylesForbidden;
			}
		}
	}

	if ( !validStyles ) {
		if ( dualSabers )
			Com_Printf( "WARNING: No valid saber styles for %s/%s", saber1->name, saber2->name );
		else
			Com_Printf( "WARNING: No valid saber styles for %s", saber1->name );
	}

	//using an invalid style and have at least one valid style to use, so switch to it
	else if ( styleInvalid ) {
		for ( styleNum=SS_FAST; styleNum<SS_NUM_SABER_STYLES; styleNum++ ) {
			if ( (validStyles & (1<<styleNum)) ) {
				*saberAnimLevel = styleNum;
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel ) {
	qboolean saber1Active, saber2Active;
	qboolean dualSabers = qfalse;

	if ( saber2 && saber2->model[0] )
		dualSabers = qtrue;

	if ( dualSabers ) {
		if ( saberHolstered > 1 )
			saber1Active = saber2Active = qfalse;
		else if ( saberHolstered > 0 ) {
			saber1Active = qtrue;
			saber2Active = qfalse;
		}
		else
			saber1Active = saber2Active = qtrue;
	}
	else {
		saber2Active = qfalse;
		if ( !saber1 || !saber1->model[0] )
			saber1Active = qfalse;

		//staff
		else if ( saber1->numBlades > 1 )
			saber1Active = (saberHolstered>1) ? qfalse : qtrue;

		//single
		else
			saber1Active = saberHolstered ? qfalse : qtrue;
	}

	if ( saber1Active && saber1 && saber1->model[0] && saber1->stylesForbidden ) {
		if ( (saber1->stylesForbidden & (1<<saberAnimLevel)) )
			return qfalse;
	}
	if ( dualSabers && saber2Active && saber2 && saber2->model[0] )
	{
		if ( saber2->stylesForbidden ) {
			if ( (saber2->stylesForbidden & (1<<saberAnimLevel)) )
				return qfalse;
		}
		//now: if using dual sabers, only dual and tavion (if given with this saber) are allowed
		if ( saberAnimLevel != SS_DUAL ) {
			if ( saberAnimLevel != SS_TAVION )
				return qfalse;
			else {
				//see if "tavion" style is okay
				if ( !(saber1Active && (saber1->stylesLearned & (1<<SS_TAVION)))
					|| !(saber2->stylesLearned & (1<<SS_TAVION)) )
					return qfalse;
			}
		}
	}

	return qtrue;
}

qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber ) {
	if ( saber->bladeStyle2Start > 0 && saber->numBlades > saber->bladeStyle2Start ) {
		// check if all blades are always on
		if ( (saber->saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE) && (saber->saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2) )
			return qfalse;
	}
	else {
		// check if all blades are always on
		if ( (saber->saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE) )
			return qfalse;
	}
	//you can turn some off
	return qtrue;
}

void WP_SaberSetDefaults( saberInfo_t *saber ) {
	int i;

	//Set defaults so that, if it fails, there's at least something there
	for ( i=0; i<MAX_BLADES; i++ ) {
		saber->blade[i].color = SABER_RED;
		saber->blade[i].radius = SABER_RADIUS_STANDARD;
		saber->blade[i].lengthMax = 32;
	}

	Q_strncpyz( saber->name, DEFAULT_SABER, sizeof( saber->name ) );
	Q_strncpyz( saber->fullName, "lightsaber", sizeof( saber->fullName ) );
	Q_strncpyz( saber->model, DEFAULT_SABER_MODEL, sizeof( saber->model ) );
	saber->skin					= 0;
	saber->soundOn				= BG_SoundIndex( "sound/weapons/saber/enemy_saber_on.wav" );
	saber->soundLoop			= BG_SoundIndex( "sound/weapons/saber/saberhum3.wav" );
	saber->soundOff				= BG_SoundIndex( "sound/weapons/saber/enemy_saber_off.wav" );
	saber->numBlades			= 1;
	saber->type					= SABER_SINGLE;
	saber->stylesLearned		= 0;
	saber->stylesForbidden		= 0;			// allow all styles
	saber->maxChain				= 0;			// 0 = use default behavior
	saber->forceRestrictions	= 0;
	saber->lockBonus			= 0;
	saber->parryBonus			= 0;
	saber->breakParryBonus		= 0;
	saber->breakParryBonus2		= 0;
	saber->disarmBonus			= 0;
	saber->disarmBonus2			= 0;
	saber->singleBladeStyle		= SS_NONE;		// makes it so that you use a different style if you only have the first blade active

//===NEW========================================================================================
	//done in cgame (client-side code)
	saber->saberFlags			= 0;			// see all the SFL_ flags
	saber->saberFlags2			= 0;			// see all the SFL2_ flags

	saber->spinSound			= 0;			// none - if set, plays this sound as it spins when thrown
	saber->swingSound[0]		= 0;			// none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[1]		= 0;			// none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[2]		= 0;			// none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!

	//done in game (server-side code)
	saber->moveSpeedScale		= 1.0f;			// 1.0 - you move faster/slower when using this saber
	saber->animSpeedScale		= 1.0f;			// 1.0 - plays normal attack animations faster/slower

	saber->kataMove				= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time
	saber->lungeAtkMove			= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they crouch+fwd+attack
	saber->jumpAtkUpMove		= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they jump+attack
	saber->jumpAtkFwdMove		= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they jump+fwd+attack
	saber->jumpAtkBackMove		= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they jump+back+attack
	saber->jumpAtkRightMove		= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they jump+rightattack
	saber->jumpAtkLeftMove		= LS_INVALID;	// LS_INVALID - if set, player will execute this move when they jump+left+attack
	saber->readyAnim			= -1;			// -1 - anim to use when standing idle
	saber->drawAnim				= -1;			// -1 - anim to use when drawing weapon
	saber->putawayAnim			= -1;			// -1 - anim to use when putting weapon away
	saber->tauntAnim			= -1;			// -1 - anim to use when hit "taunt"
	saber->bowAnim				= -1;			// -1 - anim to use when hit "bow"
	saber->meditateAnim			= -1;			// -1 - anim to use when hit "meditate"
	saber->flourishAnim			= -1;			// -1 - anim to use when hit "flourish"
	saber->gloatAnim			= -1;			// -1 - anim to use when hit "gloat"

	//***NOTE: you can only have a maximum of 2 "styles" of blades, so this next value, "bladeStyle2Start" is the number of the first blade to use these value on... all blades before this use the normal values above, all blades at and after this number use the secondary values below***
	saber->bladeStyle2Start		= 0;			// 0 - if set, blades from this number and higher use the following values (otherwise, they use the normal values already set)

	//***The following can be different for the extra blades - not setting them individually defaults them to the value for the whole saber (and first blade)***

	//===PRIMARY BLADES=====================
	//done in cgame (client-side code)
	saber->trailStyle			= 0;			// 0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	saber->g2MarksShader		= 0;			// none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->g2WeaponMarkShader	= 0;			// none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->hitSound[0]			= 0;			// none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hitSound[1]			= 0;			// none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hitSound[2]			= 0;			// none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->blockSound[0]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockSound[1]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockSound[2]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->bounceSound[0]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounceSound[1]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounceSound[2]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->blockEffect			= 0;			// none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	saber->hitPersonEffect		= 0;			// none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	saber->hitOtherEffect		= 0;			// none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	saber->bladeEffect			= 0;			// none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	saber->knockbackScale		= 0;			// 0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	saber->damageScale			= 1.0f;			// 1 - scale up or down the damage done by the saber
	saber->splashRadius			= 0.0f;			// 0 - radius of splashDamage
	saber->splashDamage			= 0;			// 0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	saber->splashKnockback		= 0.0f;			// 0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius

	//===SECONDARY BLADES===================
	//done in cgame (client-side code)
	saber->trailStyle2			= 0;			// 0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	saber->g2MarksShader2		= 0;			// none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->g2WeaponMarkShader2	= 0;			// none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->hit2Sound[0]			= 0;			// none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hit2Sound[1]			= 0;			// none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hit2Sound[2]			= 0;			// none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->block2Sound[0]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->block2Sound[1]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->block2Sound[2]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->bounce2Sound[0]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounce2Sound[1]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->bounce2Sound[2]		= 0;			// none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	saber->blockEffect2			= 0;			// none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	saber->hitPersonEffect2		= 0;			// none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	saber->hitOtherEffect2		= 0;			// none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	saber->bladeEffect2			= 0;			// none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	saber->knockbackScale2		= 0;			// 0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	saber->damageScale2			= 1.0f;			// 1 - scale up or down the damage done by the saber
	saber->splashRadius2		= 0.0f;			// 0 - radius of splashDamage
	saber->splashDamage2		= 0;			// 0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	saber->splashKnockback2		= 0.0f;			// 0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
//=========================================================================================================================================
}


static void Saber_ParseName( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	Q_strncpyz( saber->fullName, value, sizeof( saber->fullName ) );
}
static void Saber_ParseSaberType( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberType;
	if ( COM_ParseString( p, &value ) )
		return;
	saberType = GetIDForString( saberTable, value );
	if ( saberType >= SABER_SINGLE && saberType <= NUM_SABERS )
		saber->type = (saberType_t)saberType;
}
static void Saber_ParseSaberModel( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	Q_strncpyz( saber->model, value, sizeof( saber->model ) );
}
static void Saber_ParseCustomSkin( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->skin = trap->R_RegisterSkin( value );
}
static void Saber_ParseSoundOn( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->soundOn = BG_SoundIndex( value );
}
static void Saber_ParseSoundLoop( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->soundLoop = BG_SoundIndex( value );
}
static void Saber_ParseSoundOff( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->soundOff = BG_SoundIndex( value );
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
static void Saber_ParseSaberColor( saberInfo_t *saber, const char **p ) {
	const char *value;
	int i=0;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
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

	color = TranslateSaberColor( value );
	saber->blade[1].color = color;
}
static void Saber_ParseSaberColor3( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	color = TranslateSaberColor( value );
	saber->blade[2].color = color;
}
static void Saber_ParseSaberColor4( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	color = TranslateSaberColor( value );
	saber->blade[3].color = color;
}
static void Saber_ParseSaberColor5( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	color = TranslateSaberColor( value );
	saber->blade[4].color = color;
}
static void Saber_ParseSaberColor6( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
		return;

	color = TranslateSaberColor( value );
	saber->blade[5].color = color;
}
static void Saber_ParseSaberColor7( saberInfo_t *saber, const char **p ) {
	const char *value;
	saber_colors_t color;

	if ( COM_ParseString( p, &value ) )
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
	//saber->brokenSaber1 = G_NewString( value );
}
static void Saber_ParseBrokenSaber2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	//saber->brokenSaber2 = G_NewString( value );
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
	saber->spinSound = BG_SoundIndex( value );
}
static void Saber_ParseSwingSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->swingSound[0] = BG_SoundIndex( value );
}
static void Saber_ParseSwingSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->swingSound[1] = BG_SoundIndex( value );
}
static void Saber_ParseSwingSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->swingSound[2] = BG_SoundIndex( value );
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
	saberMove = GetIDForString( saberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->kataMove = saberMove; //LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time
}
static void Saber_ParseLungeAtkMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( saberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->lungeAtkMove = saberMove;
}
static void Saber_ParseJumpAtkUpMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( saberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkUpMove = saberMove;
}
static void Saber_ParseJumpAtkFwdMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( saberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkFwdMove = saberMove;
}
static void Saber_ParseJumpAtkBackMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( saberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkBackMove = saberMove;
}
static void Saber_ParseJumpAtkRightMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( saberMoveTable, value );
	if ( saberMove >= LS_INVALID && saberMove < LS_MOVE_MAX )
		saber->jumpAtkRightMove = saberMove;
}
static void Saber_ParseJumpAtkLeftMove( saberInfo_t *saber, const char **p ) {
	const char *value;
	int saberMove = LS_INVALID;
	if ( COM_ParseString( p, &value ) )
		return;
	saberMove = GetIDForString( saberMoveTable, value );
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
static void Saber_ParseOnInWater( saberInfo_t *saber, const char **p ) {
	SkipRestOfLine( p );
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
#ifdef _CGAME
	saber->g2MarksShader = trap->R_RegisterShader( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseG2WeaponMarkShader( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) ) {
		SkipRestOfLine( p );
		return;
	}
#ifdef _CGAME
	saber->g2WeaponMarkShader = trap->R_RegisterShader( value );
#else
	SkipRestOfLine( p );
#endif
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
	saber->hitSound[0] = BG_SoundIndex( value );
}
static void Saber_ParseHitSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitSound[1] = BG_SoundIndex( value );
}
static void Saber_ParseHitSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hitSound[2] = BG_SoundIndex( value );
}
static void Saber_ParseBlockSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockSound[0] = BG_SoundIndex( value );
}
static void Saber_ParseBlockSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockSound[1] = BG_SoundIndex( value );
}
static void Saber_ParseBlockSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->blockSound[2] = BG_SoundIndex( value );
}
static void Saber_ParseBounceSound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounceSound[0] = BG_SoundIndex( value );
}
static void Saber_ParseBounceSound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounceSound[1] = BG_SoundIndex( value );
}
static void Saber_ParseBounceSound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounceSound[2] = BG_SoundIndex( value );
}
static void Saber_ParseBlockEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->blockEffect = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseHitPersonEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->hitPersonEffect = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseHitOtherEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->hitOtherEffect = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseBladeEffect( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->bladeEffect = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
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
#ifdef _CGAME
	saber->g2MarksShader2 = trap->R_RegisterShader( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseG2WeaponMarkShader2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) ) {
		SkipRestOfLine( p );
		return;
	}
#ifdef _CGAME
	saber->g2WeaponMarkShader2 = trap->R_RegisterShader( value );
#else
	SkipRestOfLine( p );
#endif
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
	saber->hit2Sound[0] = BG_SoundIndex( value );
}
static void Saber_ParseHit2Sound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hit2Sound[1] = BG_SoundIndex( value );
}
static void Saber_ParseHit2Sound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->hit2Sound[2] = BG_SoundIndex( value );
}
static void Saber_ParseBlock2Sound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->block2Sound[0] = BG_SoundIndex( value );
}
static void Saber_ParseBlock2Sound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->block2Sound[1] = BG_SoundIndex( value );
}
static void Saber_ParseBlock2Sound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->block2Sound[2] = BG_SoundIndex( value );
}
static void Saber_ParseBounce2Sound1( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounce2Sound[0] = BG_SoundIndex( value );
}
static void Saber_ParseBounce2Sound2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounce2Sound[1] = BG_SoundIndex( value );
}
static void Saber_ParseBounce2Sound3( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
	saber->bounce2Sound[2] = BG_SoundIndex( value );
}
static void Saber_ParseBlockEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->blockEffect2 = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseHitPersonEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->hitPersonEffect2 = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseHitOtherEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->hitOtherEffect2 = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
}
static void Saber_ParseBladeEffect2( saberInfo_t *saber, const char **p ) {
	const char *value;
	if ( COM_ParseString( p, &value ) )
		return;
#ifdef _CGAME
	saber->bladeEffect2 = trap->FX_RegisterEffect( value );
#else
	SkipRestOfLine( p );
#endif
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
	{ "moveSpeedScale",			Saber_ParseMoveSpeedScale,		NULL	},
	{ "animSpeedScale",			Saber_ParseAnimSpeedScale,		NULL	},
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
	{ "onInWater",				Saber_ParseOnInWater,			NULL	},
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

qboolean WP_SaberParseParms( const char *saberName, saberInfo_t *saber ) {
	const char	*token, *p;
	char		useSaber[SABER_NAME_LENGTH];
	qboolean	triedDefault = qfalse;
	keywordHash_t *key;

	// make sure the hash table has been setup
	if ( !hashSetup )
		WP_SaberSetupKeywordHash();

	if ( !saber )
		return qfalse;

	//Set defaults so that, if it fails, there's at least something there
	WP_SaberSetDefaults( saber );

	if ( !VALIDSTRING( saberName ) ) {
		Q_strncpyz( useSaber, DEFAULT_SABER, sizeof( useSaber ) );
		triedDefault = qtrue;
	}
	else
		Q_strncpyz( useSaber, saberName, sizeof( useSaber ) );

	//try to parse it out
	p = saberParms;
	COM_BeginParseSession( "saberinfo" );

	// look for the right saber
	while ( p ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) {
			if ( !triedDefault ) {
				// fall back to default and restart, should always be there
				p = saberParms;
				COM_BeginParseSession( "saberinfo" );
				Q_strncpyz( useSaber, DEFAULT_SABER, sizeof( useSaber ) );
				triedDefault = qtrue;
			}
			else
				return qfalse;
		}

		if ( !Q_stricmp( token, useSaber ) )
			break;

		SkipBracedSection( &p, 0 );
	}

	// even the default saber isn't found?
	if ( !p )
		return qfalse;

	// got the name we're using for sure
	Q_strncpyz( saber->name, useSaber, sizeof( saber->name ) );

	if ( BG_ParseLiteral( &p, "{" ) )
		return qfalse;

	// parse the saber info block
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) {
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s' (WP_SaberParseParms)\n", useSaber );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) )
			break;

		key = KeywordHash_Find( saberParseKeywordHash, token );
		if ( key ) {
			key->func( saber, &p );
			continue;
		}

		Com_Printf( "WARNING: unknown keyword '%s' while parsing saber '%s'\n", token, useSaber );
		SkipRestOfLine( &p );
	}

	//FIXME: precache the saberModel(s)?

	return qtrue;
}

qboolean WP_SaberParseParm( const char *saberName, const char *parmname, char *saberData )
{
	const char	*token;
	const char	*value;
	const char	*p;

	if ( !saberName || !saberName[0] )
	{
		return qfalse;
	}

	//try to parse it out
	p = saberParms;
	COM_BeginParseSession("saberinfo");

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, saberName ) )
		{
			break;
		}

		SkipBracedSection( &p, 0 );
	}
	if ( !p )
	{
		return qfalse;
	}

	if ( BG_ParseLiteral( &p, "{" ) )
	{
		return qfalse;
	}

	// parse the saber info block
	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", saberName );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}

		if ( !Q_stricmp( token, parmname ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			strcpy( saberData, value );
			return qtrue;
		}

		SkipRestOfLine( &p );
		continue;
	}

	return qfalse;
}

qboolean WP_SaberValidForPlayerInMP( const char *saberName )
{
	char allowed [8]={0};
	if ( !WP_SaberParseParm( saberName, "notInMP", allowed ) )
	{//not defined, default is yes
		return qtrue;
	}
	if ( !allowed[0] )
	{//not defined, default is yes
		return qtrue;
	}
	else
	{//return value
		return ((qboolean)(atoi(allowed)==0));
	}
}

void WP_RemoveSaber( saberInfo_t *sabers, int saberNum )
{
	if ( !sabers )
	{
		return;
	}
	//reset everything for this saber just in case
	WP_SaberSetDefaults( &sabers[saberNum] );

	strcpy(sabers[saberNum].name, "none");
	sabers[saberNum].model[0] = 0;

	//ent->client->ps.dualSabers = qfalse;
	BG_SI_Deactivate(&sabers[saberNum]);
	BG_SI_SetLength(&sabers[saberNum], 0.0f);
//	if ( ent->weaponModel[saberNum] > 0 )
//	{
//		trap->G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[saberNum] );
//		ent->weaponModel[saberNum] = -1;
//	}
//	if ( saberNum == 1 )
//	{
//		ent->client->ps.dualSabers = qfalse;
//	}
}

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName )
{
	if ( !sabers )
	{
		return;
	}
	if ( Q_stricmp( "none", saberName ) == 0 || Q_stricmp( "remove", saberName ) == 0 )
	{
		if (saberNum != 0)
		{ //can't remove saber 0 ever
			WP_RemoveSaber( sabers, saberNum );
		}
		return;
	}

	if ( entNum < MAX_CLIENTS &&
		!WP_SaberValidForPlayerInMP( saberName ) )
	{
		WP_SaberParseParms( DEFAULT_SABER, &sabers[saberNum] );//get saber info
	}
	else
	{
		WP_SaberParseParms( saberName, &sabers[saberNum] );//get saber info
	}
	if ((sabers[1].saberFlags&SFL_TWO_HANDED))
	{//not allowed to use a 2-handed saber as second saber
		WP_RemoveSaber( sabers, 1 );
		return;
	}
	else if ((sabers[0].saberFlags&SFL_TWO_HANDED) &&
		sabers[1].model[0])
	{ //you can't use a two-handed saber with a second saber, so remove saber 2
		WP_RemoveSaber( sabers, 1 );
		return;
	}
}

void WP_SaberSetColor( saberInfo_t *sabers, int saberNum, int bladeNum, char *colorName )
{
	if ( !sabers )
	{
		return;
	}
	sabers[saberNum].blade[bladeNum].color = TranslateSaberColor( colorName );
}

static char bgSaberParseTBuffer[MAX_SABER_DATA_SIZE];

void WP_SaberLoadParms( void )
{
	int				len, totallen, saberExtFNLen, fileCnt, i;
	char			*holdChar, *marker;
	char			saberExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t	f;

	len = 0;

	//remember where to store the next one
	totallen = len;
	marker = saberParms+totallen;
	*marker = 0;

	//now load in the extra .sab extensions
	fileCnt = trap->FS_GetFileList( "ext_data/sabers", ".sab", saberExtensionListBuf, sizeof( saberExtensionListBuf ) );

	holdChar = saberExtensionListBuf;
	for ( i=0; i<fileCnt; i++, holdChar += saberExtFNLen+1 ) {
		saberExtFNLen = strlen( holdChar );

		len = trap->FS_Open( va( "ext_data/sabers/%s", holdChar ), &f, FS_READ );

		if ( !f ) {
			Com_Printf( "WP_SaberLoadParms: error reading file: %s\n", holdChar );
			continue;
		}

		if ( (totallen + len+1) >= MAX_SABER_DATA_SIZE ) {
			trap->FS_Close( f );
#ifdef UI_BUILD
			Com_Error( ERR_FATAL, "WP_SaberLoadParms: Saber extensions (*.sab) are too large!\nRan out of space before reading %s", holdChar );
#else
			Com_Error( ERR_DROP, "WP_SaberLoadParms: Saber extensions (*.sab) are too large!\nRan out of space before reading %s", holdChar );
#endif
		}

		trap->FS_Read(bgSaberParseTBuffer, len, f);
		bgSaberParseTBuffer[len] = 0;

		len = COM_Compress( bgSaberParseTBuffer );

		Q_strcat( marker, MAX_SABER_DATA_SIZE-totallen, bgSaberParseTBuffer );
		trap->FS_Close(f);

		//get around the stupid problem of not having an endline at the bottom
		//of a sab file -rww
		Q_strcat(marker, MAX_SABER_DATA_SIZE-totallen, "\n");
		len++;

		totallen += len;
		marker = saberParms+totallen;
	}
}

#ifdef UI_BUILD
qboolean WP_IsSaberTwoHanded( const char *saberName )
{
	int twoHanded;
	char	twoHandedString[8]={0};
	WP_SaberParseParm( saberName, "twoHanded", twoHandedString );
	if ( !twoHandedString[0] )
	{//not defined defaults to "no"
		return qfalse;
	}
	twoHanded = atoi( twoHandedString );
	return ((qboolean)(twoHanded!=0));
}

void WP_SaberGetHiltInfo( const char *singleHilts[MAX_SABER_HILTS], const char *staffHilts[MAX_SABER_HILTS] )
{
	int	numSingleHilts = 0, numStaffHilts = 0;
	const char	*saberName;
	const char	*token;
	const char	*p;

	//go through all the loaded sabers and put the valid ones in the proper list
	p = saberParms;
	COM_BeginParseSession("saberlist");

	// look for a saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{//invalid name
			continue;
		}
		saberName = String_Alloc( token );
		//see if there's a "{" on the next line
		SkipRestOfLine( &p );

		if ( BG_ParseLiteralSilent( &p, "{" ) )
		{//nope, not a name, keep looking
			continue;
		}

		//this is a saber name
		if ( !WP_SaberValidForPlayerInMP( saberName ) )
		{
			SkipBracedSection( &p, 0 );
			continue;
		}

		if ( WP_IsSaberTwoHanded( saberName ) )
		{
			if ( numStaffHilts < MAX_SABER_HILTS-1 )//-1 because we have to NULL terminate the list
			{
				staffHilts[numStaffHilts++] = saberName;
			}
			else
			{
				Com_Printf( "WARNING: too many two-handed sabers, ignoring saber '%s'\n", saberName );
			}
		}
		else
		{
			if ( numSingleHilts < MAX_SABER_HILTS-1 )//-1 because we have to NULL terminate the list
			{
				singleHilts[numSingleHilts++] = saberName;
			}
			else
			{
				Com_Printf( "WARNING: too many one-handed sabers, ignoring saber '%s'\n", saberName );
			}
		}
		//skip the whole braced section and move on to the next entry
		SkipBracedSection( &p, 0 );
	}
	//null terminate the list so the UI code knows where to stop listing them
	singleHilts[numSingleHilts] = NULL;
	staffHilts[numStaffHilts] = NULL;
}
#endif

/*
rww -
The following were struct functions in SP. Of course
we can't have that in this codebase so I'm having to
externalize them. Which is why this probably seems
structured a bit oddly. But it's to make porting stuff
easier on myself. SI indicates it was under saberinfo,
and BLADE indicates it was under bladeinfo.
*/

//---------------------------------------
void BG_BLADE_ActivateTrail ( bladeInfo_t *blade, float duration )
{
	blade->trail.inAction = qtrue;
	blade->trail.duration = duration;
}

void BG_BLADE_DeactivateTrail ( bladeInfo_t *blade, float duration )
{
	blade->trail.inAction = qfalse;
	blade->trail.duration = duration;
}
//---------------------------------------
void BG_SI_Activate( saberInfo_t *saber )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		saber->blade[i].active = qtrue;
	}
}

void BG_SI_Deactivate( saberInfo_t *saber )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		saber->blade[i].active = qfalse;
	}
}

// Description: Activate a specific Blade of this Saber.
// Created: 10/03/02 by Aurelio Reis, Modified: 10/03/02 by Aurelio Reis.
//	[in]		int iBlade		Which Blade to activate.
//	[in]		bool bActive	Whether to activate it (default true), or deactivate it (false).
//	[return]	void
void BG_SI_BladeActivate( saberInfo_t *saber, int iBlade, qboolean bActive )
{
	// Validate blade ID/Index.
	if ( iBlade < 0 || iBlade >= saber->numBlades )
		return;

	saber->blade[iBlade].active = bActive;
}

qboolean BG_SI_Active(saberInfo_t *saber)
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].active )
		{
			return qtrue;
		}
	}
	return qfalse;
}

void BG_SI_SetLength( saberInfo_t *saber, float length )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		saber->blade[i].length = length;
	}
}

//not in sp, added it for my own convenience
void BG_SI_SetDesiredLength(saberInfo_t *saber, float len, int bladeNum )
{
	int i, startBlade = 0, maxBlades = saber->numBlades;

	if ( bladeNum >= 0 && bladeNum < saber->numBlades)
	{//doing this on a specific blade
		startBlade = bladeNum;
		maxBlades = bladeNum+1;
	}
	for (i = startBlade; i < maxBlades; i++)
	{
		saber->blade[i].desiredLength = len;
	}
}

//also not in sp, added it for my own convenience
void BG_SI_SetLengthGradual(saberInfo_t *saber, int time)
{
	int i;
	float amt, dLen;

	for (i = 0; i < saber->numBlades; i++)
	{
		dLen = saber->blade[i].desiredLength;

		if (dLen == -1)
		{ //assume we want max blade len
			dLen = saber->blade[i].lengthMax;
		}

		if (saber->blade[i].length == dLen)
		{
			continue;
		}

		if (saber->blade[i].length == saber->blade[i].lengthMax ||
			saber->blade[i].length == 0)
		{
			saber->blade[i].extendDebounce = time;
			if (saber->blade[i].length == 0)
			{
				saber->blade[i].length++;
			}
			else
			{
				saber->blade[i].length--;
			}
		}

		amt = (time - saber->blade[i].extendDebounce)*0.01;

		if (amt < 0.2f)
		{
			amt = 0.2f;
		}

		if (saber->blade[i].length < dLen)
		{
			saber->blade[i].length += amt;

			if (saber->blade[i].length > dLen)
			{
				saber->blade[i].length = dLen;
			}
			if (saber->blade[i].length > saber->blade[i].lengthMax)
			{
				saber->blade[i].length = saber->blade[i].lengthMax;
			}
		}
		else if (saber->blade[i].length > dLen)
		{
			saber->blade[i].length -= amt;

			if (saber->blade[i].length < dLen)
			{
				saber->blade[i].length = dLen;
			}
			if (saber->blade[i].length < 0)
			{
				saber->blade[i].length = 0;
			}
		}
	}
}

float BG_SI_Length(saberInfo_t *saber)
{//return largest length
	int len1 = 0;
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].length > len1 )
		{
			len1 = saber->blade[i].length;
		}
	}
	return len1;
}

float BG_SI_LengthMax(saberInfo_t *saber)
{
	int len1 = 0;
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].lengthMax > len1 )
		{
			len1 = saber->blade[i].lengthMax;
		}
	}
	return len1;
}

void BG_SI_ActivateTrail ( saberInfo_t *saber, float duration )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		//saber->blade[i].ActivateTrail( duration );
		BG_BLADE_ActivateTrail(&saber->blade[i], duration);
	}
}

void BG_SI_DeactivateTrail ( saberInfo_t *saber, float duration )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		//saber->blade[i].DeactivateTrail( duration );
		BG_BLADE_DeactivateTrail(&saber->blade[i], duration);
	}
}

