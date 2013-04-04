//bg_saberLoad.c
#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "w_saber.h"

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

//Could use strap stuff but I don't particularly care at the moment anyway.
#include "../namespace_begin.h"
extern int	trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
extern void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
extern void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
extern void	trap_FS_FCloseFile( fileHandle_t f );
extern int	trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
extern qhandle_t trap_R_RegisterSkin( const char *name );
#include "../namespace_end.h"


#ifdef QAGAME
extern int G_SoundIndex( const char *name );
#elif defined CGAME
#include "../namespace_begin.h"
sfxHandle_t trap_S_RegisterSound( const char *sample);
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
int	trap_FX_RegisterEffect(const char *file);
#include "../namespace_end.h"
#endif

#include "../namespace_begin.h"

int BG_SoundIndex(char *sound)
{
#ifdef QAGAME
	return G_SoundIndex(sound);
#elif defined CGAME
	return trap_S_RegisterSound(sound);
#endif
}

extern stringID_table_t FPTable[];

#define MAX_SABER_DATA_SIZE 0x80000
static char SaberParms[MAX_SABER_DATA_SIZE];

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
	"",	-1
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
	"",	-1
};

//Also used in npc code
qboolean BG_ParseLiteral( const char **data, const char *string ) 
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) 
	{
		Com_Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
}

saber_colors_t TranslateSaberColor( const char *name ) 
{
	if ( !Q_stricmp( name, "red" ) ) 
	{
		return SABER_RED;
	}
	if ( !Q_stricmp( name, "orange" ) ) 
	{
		return SABER_ORANGE;
	}
	if ( !Q_stricmp( name, "yellow" ) ) 
	{
		return SABER_YELLOW;
	}
	if ( !Q_stricmp( name, "green" ) ) 
	{
		return SABER_GREEN;
	}
	if ( !Q_stricmp( name, "blue" ) ) 
	{
		return SABER_BLUE;
	}
	if ( !Q_stricmp( name, "purple" ) ) 
	{
		return SABER_PURPLE;
	}
	if ( !Q_stricmp( name, "random" ) ) 
	{
		return ((saber_colors_t)(Q_irand( SABER_ORANGE, SABER_PURPLE )));
	}
	return SABER_BLUE;
}

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

qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel )
{
	qboolean styleInvalid = qfalse;
	qboolean saber1Active;
	qboolean saber2Active;
	qboolean dualSabers = qfalse;
	int	validStyles = 0, styleNum;

	if ( saber2 && saber2->model && saber2->model[0] )
	{
		dualSabers = qtrue;
	}

	if ( dualSabers )
	{//dual
		if ( saberHolstered > 1 )
		{
			saber1Active = saber2Active = qfalse;
		}
		else if ( saberHolstered > 0 )
		{
			saber1Active = qtrue;
			saber2Active = qfalse;
		}
		else
		{
			saber1Active = saber2Active = qtrue;
		}
	}
	else
	{
		saber2Active = qfalse;
		if ( !saber1
			|| !saber1->model
			|| !saber1->model[0] )
		{
			saber1Active = qfalse;
		}
		else if ( saber1->numBlades > 1 )
		{//staff
			if ( saberHolstered > 1 )
			{
				saber1Active = qfalse;
			}
			else
			{
				saber1Active = qtrue;
			}
		}
		else
		{//single
			if ( saberHolstered )
			{
				saber1Active = qfalse;
			}
			else
			{
				saber1Active = qtrue;
			}
		}
	}

	//initially, all styles are valid
	for ( styleNum = SS_NONE+1; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
	{
		validStyles |= (1<<styleNum);
	}

	if ( saber1Active
		&& saber1
		&& saber1->model
		&& saber1->model[0]
		&& saber1->stylesForbidden )
	{
		if ( (saber1->stylesForbidden&(1<<*saberAnimLevel)) )
		{//not a valid style for first saber!
			styleInvalid = qtrue;
			validStyles &= ~saber1->stylesForbidden;
		}
	}
	if ( dualSabers )
	{//check second saber, too
		if ( saber2Active
			&& saber2->stylesForbidden )
		{
			if ( (saber2->stylesForbidden&(1<<*saberAnimLevel)) )
			{//not a valid style for second saber!
				styleInvalid = qtrue;
				//only the ones both sabers allow is valid
				validStyles &= ~saber2->stylesForbidden;
			}
		}
	}
	if ( styleInvalid && validStyles )
	{//using an invalid style and have at least one valid style to use, so switch to it
		int styleNum;
		for ( styleNum = SS_FAST; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
		{
			if ( (validStyles&(1<<styleNum)) )
			{
				*saberAnimLevel = styleNum;
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel )
{
	qboolean saber1Active;
	qboolean saber2Active;
	qboolean dualSabers = qfalse;

	if ( saber2 && saber2->model && saber2->model[0] )
	{
		dualSabers = qtrue;
	}

	if ( dualSabers )
	{//dual
		if ( saberHolstered > 1 )
		{
			saber1Active = saber2Active = qfalse;
		}
		else if ( saberHolstered > 0 )
		{
			saber1Active = qtrue;
			saber2Active = qfalse;
		}
		else
		{
			saber1Active = saber2Active = qtrue;
		}
	}
	else
	{
		saber2Active = qfalse;
		if ( !saber1
			|| !saber1->model
			|| !saber1->model[0] )
		{
			saber1Active = qfalse;
		}
		else if ( saber1->numBlades > 1 )
		{//staff
			if ( saberHolstered > 1 )
			{
				saber1Active = qfalse;
			}
			else
			{
				saber1Active = qtrue;
			}
		}
		else
		{//single
			if ( saberHolstered )
			{
				saber1Active = qfalse;
			}
			else
			{
				saber1Active = qtrue;
			}
		}
	}

	if ( saber1Active
		&& saber1
		&& saber1->model
		&& saber1->model[0]
		&& saber1->stylesForbidden )
	{
		if ( (saber1->stylesForbidden&(1<<saberAnimLevel)) )
		{//not a valid style for first saber!
			return qfalse;
		}
	}
	if ( dualSabers
		&& saber2Active
		&& saber2
		&& saber2->model
		&& saber2->model[0] )
	{
		if ( saber2->stylesForbidden )
		{//check second saber, too
			if ( (saber2->stylesForbidden&(1<<saberAnimLevel)) )
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
				if ( saber1Active
					&& saber1
					&& saber1->model
					&& saber1->model[0]
					&& (saber1->stylesLearned&(1<<SS_TAVION)) )
				{//okay to use tavion style, first saber gave it to us
				}
				else if ( (saber2->stylesLearned&(1<<SS_TAVION)) )
				{//okay to use tavion style, second saber gave it to us
				}
				else
				{//tavion style is not allowed because neither of the sabers we're using gave it to us (I know, doesn't quite make sense, but...)
					return qfalse;
				}
			}
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

void WP_SaberSetDefaults( saberInfo_t *saber )
{
	int i;

	//Set defaults so that, if it fails, there's at least something there
	for ( i = 0; i < MAX_BLADES; i++ )
	{
		saber->blade[i].color = SABER_RED;
		saber->blade[i].radius = SABER_RADIUS_STANDARD;
		saber->blade[i].lengthMax = 32;
	}

	strcpy(saber->name, "default");
	strcpy(saber->fullName, "lightsaber");
	strcpy(saber->model, "models/weapons2/saber_reborn/saber_w.glm");
	saber->skin = 0;
	saber->soundOn = BG_SoundIndex( "sound/weapons/saber/enemy_saber_on.wav" );
	saber->soundLoop = BG_SoundIndex( "sound/weapons/saber/saberhum3.wav" );
	saber->soundOff = BG_SoundIndex( "sound/weapons/saber/enemy_saber_off.wav" );
	saber->numBlades = 1;
	saber->type = SABER_SINGLE;
	saber->stylesLearned = 0;
	saber->stylesForbidden = 0;//allow all styles
	saber->maxChain = 0;//0 = use default behavior
	saber->forceRestrictions = 0;
	saber->lockBonus = 0;
	saber->parryBonus = 0;
	saber->breakParryBonus = 0;
	saber->breakParryBonus2 = 0;
	saber->disarmBonus = 0;
	saber->disarmBonus2 = 0;
	saber->singleBladeStyle = SS_NONE;//makes it so that you use a different style if you only have the first blade active
//	saber->brokenSaber1 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
//	saber->brokenSaber2 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
//===NEW========================================================================================
	//done in cgame (client-side code)
	saber->saberFlags = 0;					//see all the SFL_ flags
	saber->saberFlags2 = 0;					//see all the SFL2_ flags

	saber->spinSound = 0;					//none - if set, plays this sound as it spins when thrown
	saber->swingSound[0] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[1] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[2] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!

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
	saber->g2MarksShader = 0;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->g2WeaponMarkShader = 0;			//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
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
	saber->g2MarksShader2 = 0;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	saber->g2WeaponMarkShader2 = 0;			//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
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

#define DEFAULT_SABER "Kyle"

qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber ) 
{
	const char	*token;
	const char	*value;
	const char	*p;
	char	useSaber[1024];
	float	f;
	int		n;
	qboolean	triedDefault = qfalse;
	int saberMove = LS_INVALID;
	int	anim = -1;
	
	if ( !saber ) 
	{
		return qfalse;
	}
	
	//Set defaults so that, if it fails, there's at least something there
	WP_SaberSetDefaults( saber );

	if ( !SaberName || !SaberName[0] ) 
	{
		strcpy(useSaber, DEFAULT_SABER); //default
		triedDefault = qtrue;
	}
	else
	{
		strcpy(useSaber, SaberName);
	}

	//try to parse it out
	p = SaberParms;
	COM_BeginParseSession("saberinfo");

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			if (!triedDefault)
			{ //fall back to default and restart, should always be there
				p = SaberParms;
				COM_BeginParseSession("saberinfo");
				strcpy(useSaber, DEFAULT_SABER);
				triedDefault = qtrue;
			}
			else
			{
				return qfalse;
			}
		}

		if ( !Q_stricmp( token, useSaber ) ) 
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p ) 
	{ //even the default saber isn't found?
		return qfalse;
	}

	//got the name we're using for sure
	strcpy(saber->name, useSaber);

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
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", useSaber );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) ) 
		{
			break;
		}

		//saber fullName
		if ( !Q_stricmp( token, "name" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			strcpy(saber->fullName, value);
			continue;
		}

		//saber type
		if ( !Q_stricmp( token, "saberType" ) ) 
		{
			int saberType;

			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saberType = GetIDForString( SaberTable, value );
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
			strcpy(saber->model, value);
			continue;
		}

		if ( !Q_stricmp( token, "customSkin" ) )
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->skin = trap_R_RegisterSkin(value);
			continue;
		}

		//on sound
		if ( !Q_stricmp( token, "soundOn" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOn = BG_SoundIndex( (char *)value );
			continue;
		}

		//loop sound
		if ( !Q_stricmp( token, "soundLoop" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundLoop = BG_SoundIndex( (char *)value );
			continue;
		}

		//off sound
		if ( !Q_stricmp( token, "soundOff" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOff = BG_SoundIndex( (char *)value );
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
				Com_Error(ERR_DROP, "WP_SaberParseParms: saber %s has illegal number of blades (%d) max: %d", useSaber, n, MAX_BLADES );
				continue;
			}
			saber->numBlades = n;
			continue;
		}

		// saberColor
		if ( !Q_stricmpn( token, "saberColor", 10 ) ) 
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
					Com_Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, useSaber );
					continue;
				}
			}
			else
			{
				Com_Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, useSaber );
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
					Com_Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, useSaber );
					continue;
				}
			}
			else
			{
				Com_Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, useSaber );
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
					Com_Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, useSaber );
					continue;
				}
			}
			else
			{
				Com_Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, useSaber );
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
			int fp;

			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			fp = GetIDForString( FPTable, value );
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
			//saber->brokenSaber1 = G_NewString( value );
			continue;
		}
		
		//broken replacement saber2 (left hand)
		if ( !Q_stricmp( token, "brokenSaber2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			//saber->brokenSaber2 = G_NewString( value );
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
			saber->spinSound = BG_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[2] = BG_SoundIndex( (char *)value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			saberMove = GetIDForString( SaberMoveTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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
			anim = GetIDForString( animTable, value );
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

		//stays on in water
		if ( !Q_stricmp( token, "onInWater" ) ) 
		{//ignore in MP
			SkipRestOfLine( &p );
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
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->g2MarksShader = trap_R_RegisterShader( value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
		if ( !Q_stricmp( token, "g2WeaponMarkShader" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->g2WeaponMarkShader = trap_R_RegisterShader( value );
#else
			SkipRestOfLine(&p);
#endif
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
			saber->hitSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounceSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounceSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounceSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounceSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounceSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounceSound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block effect - when saber/sword hits another saber/sword
		if ( !Q_stricmp( token, "blockEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->blockEffect = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//hit person effect - when saber/sword hits a person
		if ( !Q_stricmp( token, "hitPersonEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->hitPersonEffect = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//hit other effect - when saber/sword hits sopmething else damagable
		if ( !Q_stricmp( token, "hitOtherEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->hitOtherEffect = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//blade effect
		if ( !Q_stricmp( token, "bladeEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->bladeEffect = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
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
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->g2MarksShader2 = trap_R_RegisterShader( value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
		if ( !Q_stricmp( token, "g2WeaponMarkShader2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->g2WeaponMarkShader2 = trap_R_RegisterShader( value );
#else
			SkipRestOfLine(&p);
#endif
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
			saber->hit2Sound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hit2Sound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hit2Sound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hit2Sound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hit2Sound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "block2Sound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->block2Sound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "block2Sound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->block2Sound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "block2Sound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->block2Sound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounce2Sound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounce2Sound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounce2Sound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounce2Sound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//bounce sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "bounce2Sound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->bounce2Sound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block effect - when saber/sword hits another saber/sword
		if ( !Q_stricmp( token, "blockEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->blockEffect2 = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//hit person effect - when saber/sword hits a person
		if ( !Q_stricmp( token, "hitPersonEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->hitPersonEffect2 = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//hit other effect - when saber/sword hits sopmething else damagable
		if ( !Q_stricmp( token, "hitOtherEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->hitOtherEffect2 = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
			continue;
		}

		//blade effect
		if ( !Q_stricmp( token, "bladeEffect2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
			SkipRestOfLine(&p);
#elif defined CGAME
			saber->bladeEffect2 = trap_FX_RegisterEffect( (char *)value );
#else
			SkipRestOfLine(&p);
#endif
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

		//FIXME: saber sounds (on, off, loop)

#ifdef _DEBUG
		Com_Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, useSaber );
#endif
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
	p = SaberParms;
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

		SkipBracedSection( &p );
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
//		gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[saberNum] );
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
		WP_SaberParseParms( "Kyle", &sabers[saberNum] );//get saber info
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
	int			len, totallen, saberExtFNLen, mainBlockLen, fileCnt, i;
	//const char	*filename = "ext_data/sabers.cfg";
	char		*holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t	f;

	len = 0;

	//remember where to store the next one
	totallen = mainBlockLen = len;
	marker = SaberParms+totallen;
	*marker = 0;

	//now load in the extra .sab extensions
	fileCnt = trap_FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) 
	{
		saberExtFNLen = strlen( holdChar );

		len = trap_FS_FOpenFile(va( "ext_data/sabers/%s", holdChar), &f, FS_READ);

		if ( len == -1 ) 
		{
			Com_Printf( "error reading file\n" );
		}
		else
		{
			if ( (totallen + len + 1/*for the endline*/) >= MAX_SABER_DATA_SIZE ) {
				Com_Error(ERR_DROP, "Saber extensions (*.sab) are too large" );
			}

			trap_FS_Read(bgSaberParseTBuffer, len, f);
			bgSaberParseTBuffer[len] = 0;

			len = COM_Compress( bgSaberParseTBuffer );

			Q_strcat( marker, MAX_SABER_DATA_SIZE-totallen, bgSaberParseTBuffer );
			trap_FS_FCloseFile(f);

			//get around the stupid problem of not having an endline at the bottom
			//of a sab file -rww
			Q_strcat(marker, MAX_SABER_DATA_SIZE-totallen, "\n");
			len++;

			totallen += len;
			marker = SaberParms+totallen;
		}
	}
}

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

#include "../namespace_end.h"
