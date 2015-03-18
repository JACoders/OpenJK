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

#include "common_headers.h"


// define GAME_INCLUDE so that g_public.h does not define the
// short, server-visible gclient_t and gentity_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE

#include "../qcommon/q_shared.h"
#include "g_shared.h"
#include "bg_local.h"
#include "../cgame/cg_local.h"
#include "anims.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "wp_saber.h"
#include "g_vehicles.h"

extern pmove_t	*pm;
extern pml_t	pml;
extern cvar_t	*g_ICARUSDebug;
extern cvar_t	*g_timescale;
extern cvar_t	*g_synchSplitAnims;
extern cvar_t	*g_AnimWarning;
extern cvar_t	*g_noFootSlide;
extern cvar_t	*g_noFootSlideRunScale;
extern cvar_t	*g_noFootSlideWalkScale;
extern cvar_t	*g_saberAnimSpeed;
extern cvar_t	*g_saberAutoAim;
extern cvar_t	*g_speederControlScheme;
extern cvar_t	*g_saberNewControlScheme;

extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern qboolean ValidAnimFileIndex ( int index );
extern qboolean PM_ControlledByPlayer( void );
extern qboolean PM_DroidMelee( int npc_class );
extern qboolean PM_PainAnim( int anim );
extern qboolean PM_JumpingAnim( int anim );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_SwimmingAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_InRoll( playerState_t *ps );
extern qboolean PM_DodgeAnim( int anim );
extern qboolean PM_InSlopeAnim( int anim );
extern qboolean PM_ForceAnim( int anim );
extern qboolean PM_InKnockDownOnGround( playerState_t *ps );
extern qboolean PM_InSpecialJump( int anim );
extern qboolean PM_RunningAnim( int anim );
extern qboolean PM_WalkingAnim( int anim );
extern qboolean PM_SwimmingAnim( int anim );
extern qboolean PM_JumpingAnim( int anim );
extern qboolean PM_SaberStanceAnim( int anim );
extern qboolean PM_SaberDrawPutawayAnim( int anim );
extern void PM_SetJumped( float height, qboolean force );
extern qboolean PM_InGetUpNoRoll( playerState_t *ps );
extern qboolean PM_CrouchAnim( int anim );
extern qboolean G_TryingKataAttack( gentity_t *self, usercmd_t *cmd );
extern qboolean G_TryingCartwheel( gentity_t *self, usercmd_t *cmd );
extern qboolean G_TryingSpecial( gentity_t *self, usercmd_t *cmd );
extern qboolean G_TryingJumpAttack( gentity_t *self, usercmd_t *cmd );
extern qboolean G_TryingJumpForwardAttack( gentity_t *self, usercmd_t *cmd );
extern qboolean G_TryingLungeAttack( gentity_t *self, usercmd_t *cmd );
extern qboolean G_TryingPullAttack( gentity_t *self, usercmd_t *cmd, qboolean amPulling );
extern qboolean G_InCinematicSaberAnim( gentity_t *self );
extern qboolean G_ControlledByPlayer( gentity_t *self );

extern int g_crosshairEntNum;

int PM_AnimLength( int index, animNumber_t anim );
qboolean PM_LockedAnim( int anim );
qboolean PM_StandingAnim( int anim );
qboolean PM_InOnGroundAnim ( playerState_t *ps );
qboolean PM_SuperBreakWinAnim( int anim );
qboolean PM_SuperBreakLoseAnim( int anim );
qboolean PM_LockedAnim( int anim );
saberMoveName_t PM_SaberFlipOverAttackMove( void );
qboolean PM_CheckFlipOverAttackMove( qboolean checkEnemy );
saberMoveName_t PM_SaberJumpForwardAttackMove( void );
qboolean PM_CheckJumpForwardAttackMove( void );
saberMoveName_t PM_SaberBackflipAttackMove( void );
qboolean PM_CheckBackflipAttackMove( void );
saberMoveName_t PM_SaberDualJumpAttackMove( void );
qboolean PM_CheckDualJumpAttackMove( void );
saberMoveName_t PM_SaberLungeAttackMove( qboolean fallbackToNormalLunge );
qboolean PM_CheckLungeAttackMove( void );
// Okay, here lies the much-dreaded Pat-created FSM movement chart...  Heretic II strikes again!
// Why am I inflicting this on you?  Well, it's better than hardcoded states.
// Ideally this will be replaced with an external file or more sophisticated move-picker
// once the game gets out of prototype stage.

// Silly, but I'm replacing these macros so they are shorter!
#define AFLAG_IDLE	(SETANIM_FLAG_NORMAL)
#define AFLAG_ACTIVE (SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_WAIT (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_FINISH (SETANIM_FLAG_HOLD)

//FIXME: add the alternate anims for each style?
saberMoveData_t	saberMoveData[LS_MOVE_MAX] = {//							NB:randomized
	// name			anim(do all styles?)startQ	endQ	setanimflag		blend,	blocking	chain_idle		chain_attack	trailLen
	{"None",		BOTH_STAND1,		Q_R,	Q_R,	AFLAG_IDLE,		350,	BLK_NO,		LS_NONE,		LS_NONE,		0	},	// LS_NONE		= 0,

	// General movements with saber
	{"Ready",		BOTH_STAND2,		Q_R,	Q_R,	AFLAG_IDLE,		350,	BLK_WIDE,	LS_READY,		LS_S_R2L,		0	},	// LS_READY,
	{"Draw",		BOTH_STAND1TO2,		Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0	},	// LS_DRAW,
	{"Putaway",		BOTH_STAND2TO1,		Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0	},	// LS_PUTAWAY,

	// Attacks
	//UL2LR
	{"TL2BR Att",	BOTH_A1_TL_BR,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_TL2BR,		LS_R_TL2BR,		200	},	// LS_A_TL2BR
	//SLASH LEFT
	{"L2R Att",		BOTH_A1__L__R,		Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_L2R,		LS_R_L2R,		200 },	// LS_A_L2R
	//LL2UR
	{"BL2TR Att",	BOTH_A1_BL_TR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_TIGHT,	LS_R_BL2TR,		LS_R_BL2TR,		200	},	// LS_A_BL2TR
	//LR2UL
	{"BR2TL Att",	BOTH_A1_BR_TL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_BR2TL,		LS_R_BR2TL,		200	},	// LS_A_BR2TL
	//SLASH RIGHT
	{"R2L Att",		BOTH_A1__R__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_R2L,		LS_R_R2L,		200 },// LS_A_R2L
	//UR2LL
	{"TR2BL Att",	BOTH_A1_TR_BL,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_TR2BL,		LS_R_TR2BL,		200	},	// LS_A_TR2BL
	//SLASH DOWN
	{"T2B Att",		BOTH_A1_T__B_,		Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_T2B,		LS_R_T2B,		200	},	// LS_A_T2B
	//special attacks
	{"Back Stab",	BOTH_A2_STABBACK1,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_BACKSTAB
	{"Back Att",	BOTH_ATTACK_BACK,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_BACK
	{"CR Back Att",	BOTH_CROUCHATTACKBACK1,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_BACK_CR
	{"RollStab",	BOTH_ROLL_STAB,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_ROLL_STAB
	{"Lunge Att",	BOTH_LUNGE2_B__T_,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_LUNGE
	{"Jump Att",	BOTH_FORCELEAP2_T__B_,Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_JUMP_T__B_
	{"Flip Stab",	BOTH_JUMPFLIPSTABDOWN,Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200	},	// LS_A_FLIP_STAB
	{"Flip Slash",	BOTH_JUMPFLIPSLASHDOWN1,Q_L,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R_T_,	200	},	// LS_A_FLIP_SLASH
	{"DualJump Atk",BOTH_JUMPATTACK6,	Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_BL_TR,	200	},	// LS_JUMPATTACK_DUAL

	{"DualJumpAtkL_A",BOTH_ARIAL_LEFT,	Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_A_TL2BR,		200	},	// LS_JUMPATTACK_ARIAL_LEFT
	{"DualJumpAtkR_A",BOTH_ARIAL_RIGHT,	Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_A_TR2BL,		200	},	// LS_JUMPATTACK_ARIAL_RIGHT

	{"DualJumpAtkL_A",BOTH_CARTWHEEL_LEFT,	Q_R,Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_TL_BR,	200	},	// LS_JUMPATTACK_CART_LEFT
	{"DualJumpAtkR_A",BOTH_CARTWHEEL_RIGHT,	Q_R,Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_TR_BL,	200	},	// LS_JUMPATTACK_CART_RIGHT

	{"DualJumpAtkLStaff", BOTH_BUTTERFLY_FL1,Q_R,Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__L__R,	200	},	// LS_JUMPATTACK_STAFF_LEFT
	{"DualJumpAtkRStaff", BOTH_BUTTERFLY_FR1,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R__L,	200	},	// LS_JUMPATTACK_STAFF_RIGHT

	{"ButterflyLeft", BOTH_BUTTERFLY_LEFT,Q_R,Q_L,		AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__L__R,	200	},	// LS_BUTTERFLY_LEFT
	{"ButterflyRight", BOTH_BUTTERFLY_RIGHT,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R__L,	200	},	// LS_BUTTERFLY_RIGHT

	{"BkFlip Atk",	BOTH_JUMPATTACK7,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200	},	// LS_A_BACKFLIP_ATK
	{"DualSpinAtk",	BOTH_SPINATTACK6,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SPINATTACK_DUAL
	{"StfSpinAtk",	BOTH_SPINATTACK7,	Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SPINATTACK
	{"LngLeapAtk",	BOTH_FORCELONGLEAP_ATTACK,Q_R,Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_LEAP_ATTACK
	{"SwoopAtkR",	BOTH_VS_ATR_S,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_READY,		200	},	// LS_SWOOP_ATTACK_RIGHT
	{"SwoopAtkL",	BOTH_VS_ATL_S,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_READY,		200	},	// LS_SWOOP_ATTACK_LEFT
	{"TauntaunAtkR",BOTH_VT_ATR_S,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_TAUNTAUN_ATTACK_RIGHT
	{"TauntaunAtkL",BOTH_VT_ATL_S,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_TAUNTAUN_ATTACK_LEFT
	{"StfKickFwd",	BOTH_A7_KICK_F,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_F
	{"StfKickBack",	BOTH_A7_KICK_B,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_B
	{"StfKickRight",BOTH_A7_KICK_R,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_R
	{"StfKickLeft",	BOTH_A7_KICK_L,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_L
	{"StfKickSpin",	BOTH_A7_KICK_S,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200	},	// LS_KICK_S
	{"StfKickBkFwd",BOTH_A7_KICK_BF,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200	},	// LS_KICK_BF
	{"StfKickSplit",BOTH_A7_KICK_RL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200	},	// LS_KICK_RL
	{"StfKickFwdAir",BOTH_A7_KICK_F_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_F_AIR
	{"StfKickBackAir",BOTH_A7_KICK_B_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_B_AIR
	{"StfKickRightAir",BOTH_A7_KICK_R_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_R_AIR
	{"StfKickLeftAir",BOTH_A7_KICK_L_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_L_AIR
	{"StabDown",	BOTH_STABDOWN,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_STABDOWN
	{"StabDownStf",	BOTH_STABDOWN_STAFF,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_STABDOWN_STAFF
	{"StabDownDual",BOTH_STABDOWN_DUAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_STABDOWN_DUAL
	{"dualspinprot",BOTH_A6_SABERPROTECT,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500	},	// LS_DUAL_SPIN_PROTECT
	{"StfSoulCal",	BOTH_A7_SOULCAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500	},	// LS_STAFF_SOULCAL
	{"specialfast",	BOTH_A1_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000},	// LS_A1_SPECIAL
	{"specialmed",	BOTH_A2_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000},	// LS_A2_SPECIAL
	{"specialstr",	BOTH_A3_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000},	// LS_A3_SPECIAL
	{"upsidedwnatk",BOTH_FLIP_ATTACK7,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200},	// LS_UPSIDE_DOWN_ATTACK
	{"pullatkstab",	BOTH_PULL_IMPALE_STAB,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200},	// LS_PULL_ATTACK_STAB
	{"pullatkswing",BOTH_PULL_IMPALE_SWING,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200},	// LS_PULL_ATTACK_SWING
	{"AloraSpinAtk",BOTH_ALORA_SPIN_SLASH,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SPINATTACK_ALORA
	{"Dual FB Atk",	BOTH_A6_FB,			Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_DUAL_FB
	{"Dual LR Atk",	BOTH_A6_LR,			Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_DUAL_LR
	{"StfHiltBash",	BOTH_A7_HILT,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_HILT_BASH

	//starts
	{"TL2BR St",	BOTH_S1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_TL2BR,		LS_A_TL2BR,		200	},	// LS_S_TL2BR
	{"L2R St",		BOTH_S1_S1__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_L2R,		LS_A_L2R,		200	},	// LS_S_L2R
	{"BL2TR St",	BOTH_S1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_BL2TR,		LS_A_BL2TR,		200	},	// LS_S_BL2TR
	{"BR2TL St",	BOTH_S1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_BR2TL,		LS_A_BR2TL,		200	},	// LS_S_BR2TL
	{"R2L St",		BOTH_S1_S1__R,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_R2L,		LS_A_R2L,		200	},	// LS_S_R2L
	{"TR2BL St",	BOTH_S1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_TR2BL,		LS_A_TR2BL,		200	},	// LS_S_TR2BL
	{"T2B St",		BOTH_S1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_T2B,		LS_A_T2B,		200	},	// LS_S_T2B

	//returns
	{"TL2BR Ret",	BOTH_R1_BR_S1,		Q_BR,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_TL2BR
	{"L2R Ret",		BOTH_R1__R_S1,		Q_R,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_L2R
	{"BL2TR Ret",	BOTH_R1_TR_S1,		Q_TR,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_BL2TR
	{"BR2TL Ret",	BOTH_R1_TL_S1,		Q_TL,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_BR2TL
	{"R2L Ret",		BOTH_R1__L_S1,		Q_L,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_R2L
	{"TR2BL Ret",	BOTH_R1_BL_S1,		Q_BL,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_TR2BL
	{"T2B Ret",		BOTH_R1_B__S1,		Q_B,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_T2B

	//Transitions
	{"BR2R Trans",	BOTH_T1_BR__R,		Q_BR,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc bottom right to right
	{"BR2TR Trans",	BOTH_T1_BR_TR,		Q_BR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc bottom right to top right		(use: BOTH_T1_TR_BR)
	{"BR2T Trans",	BOTH_T1_BR_T_,		Q_BR,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc bottom right to top			(use: BOTH_T1_T__BR)
	{"BR2TL Trans",	BOTH_T1_BR_TL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast weak spin bottom right to top left
	{"BR2L Trans",	BOTH_T1_BR__L,		Q_BR,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast weak spin bottom right to left
	{"BR2BL Trans",	BOTH_T1_BR_BL,		Q_BR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast weak spin bottom right to bottom left
	{"R2BR Trans",	BOTH_T1__R_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast arc right to bottom right			(use: BOTH_T1_BR__R)
	{"R2TR Trans",	BOTH_T1__R_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc right to top right
	{"R2T Trans",	BOTH_T1__R_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast ar right to top				(use: BOTH_T1_T___R)
	{"R2TL Trans",	BOTH_T1__R_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc right to top left
	{"R2L Trans",	BOTH_T1__R__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast weak spin right to left
	{"R2BL Trans",	BOTH_T1__R_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast weak spin right to bottom left
	{"TR2BR Trans",	BOTH_T1_TR_BR,		Q_TR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast arc top right to bottom right
	{"TR2R Trans",	BOTH_T1_TR__R,		Q_TR,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc top right to right			(use: BOTH_T1__R_TR)
	{"TR2T Trans",	BOTH_T1_TR_T_,		Q_TR,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc top right to top				(use: BOTH_T1_T__TR)
	{"TR2TL Trans",	BOTH_T1_TR_TL,		Q_TR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc top right to top left
	{"TR2L Trans",	BOTH_T1_TR__L,		Q_TR,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc top right to left
	{"TR2BL Trans",	BOTH_T1_TR_BL,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast weak spin top right to bottom left
	{"T2BR Trans",	BOTH_T1_T__BR,		Q_T,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast arc top to bottom right
	{"T2R Trans",	BOTH_T1_T___R,		Q_T,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc top to right
	{"T2TR Trans",	BOTH_T1_T__TR,		Q_T,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc top to top right
	{"T2TL Trans",	BOTH_T1_T__TL,		Q_T,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc top to top left
	{"T2L Trans",	BOTH_T1_T___L,		Q_T,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc top to left
	{"T2BL Trans",	BOTH_T1_T__BL,		Q_T,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast arc top to bottom left
	{"TL2BR Trans",	BOTH_T1_TL_BR,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast weak spin top left to bottom right
	{"TL2R Trans",	BOTH_T1_TL__R,		Q_TL,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc top left to right			(use: BOTH_T1__R_TL)
	{"TL2TR Trans",	BOTH_T1_TL_TR,		Q_TL,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc top left to top right			(use: BOTH_T1_TR_TL)
	{"TL2T Trans",	BOTH_T1_TL_T_,		Q_TL,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc top left to top				(use: BOTH_T1_T__TL)
	{"TL2L Trans",	BOTH_T1_TL__L,		Q_TL,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc top left to left				(use: BOTH_T1__L_TL)
	{"TL2BL Trans",	BOTH_T1_TL_BL,		Q_TL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast arc top left to bottom left
	{"L2BR Trans",	BOTH_T1__L_BR,		Q_L,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast weak spin left to bottom right
	{"L2R Trans",	BOTH_T1__L__R,		Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast weak spin left to right
	{"L2TR Trans",	BOTH_T1__L_TR,		Q_L,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc left to top right			(use: BOTH_T1_TR__L)
	{"L2T Trans",	BOTH_T1__L_T_,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc left to top				(use: BOTH_T1_T___L)
	{"L2TL Trans",	BOTH_T1__L_TL,		Q_L,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc left to top left
	{"L2BL Trans",	BOTH_T1__L_BL,		Q_L,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast arc left to bottom left			(use: BOTH_T1_BL__L)
	{"BL2BR Trans",	BOTH_T1_BL_BR,		Q_BL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast weak spin bottom left to bottom right
	{"BL2R Trans",	BOTH_T1_BL__R,		Q_BL,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast weak spin bottom left to right
	{"BL2TR Trans",	BOTH_T1_BL_TR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast weak spin bottom left to top right
	{"BL2T Trans",	BOTH_T1_BL_T_,		Q_BL,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc bottom left to top			(use: BOTH_T1_T__BL)
	{"BL2TL Trans",	BOTH_T1_BL_TL,		Q_BL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc bottom left to top left		(use: BOTH_T1_TL_BL)
	{"BL2L Trans",	BOTH_T1_BL__L,		Q_BL,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc bottom left to left

	//Bounces
	{"Bounce BR",	BOTH_B1_BR___,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_T1_BR_TR,	150	},
	{"Bounce R",	BOTH_B1__R___,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_T1__R__L,	150	},
	{"Bounce TR",	BOTH_B1_TR___,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_TR_TL,	150	},
	{"Bounce T",	BOTH_B1_T____,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150	},
	{"Bounce TL",	BOTH_B1_TL___,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_T1_TL_TR,	150	},
	{"Bounce L",	BOTH_B1__L___,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_T1__L__R,	150	},
	{"Bounce BL",	BOTH_B1_BL___,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_T1_BL_TR,	150	},

	//Deflected attacks (like bounces, but slide off enemy saber, not straight back)
	{"Deflect BR",	BOTH_D1_BR___,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_T1_BR_TR,	150	},
	{"Deflect R",	BOTH_D1__R___,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_T1__R__L,	150	},
	{"Deflect TR",	BOTH_D1_TR___,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_TR_TL,	150	},
	{"Deflect T",	BOTH_B1_T____,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150	},
	{"Deflect TL",	BOTH_D1_TL___,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_T1_TL_TR,	150	},
	{"Deflect L",	BOTH_D1__L___,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_T1__L__R,	150	},
	{"Deflect BL",	BOTH_D1_BL___,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_T1_BL_TR,	150	},
	{"Deflect B",	BOTH_D1_B____,		Q_B,	Q_B,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150	},

	//Reflected attacks
	{"Reflected BR",BOTH_V1_BR_S1,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_BR
	{"Reflected R",	BOTH_V1__R_S1,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1__R
	{"Reflected TR",BOTH_V1_TR_S1,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_TR
	{"Reflected T",	BOTH_V1_T__S1,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_T_
	{"Reflected TL",BOTH_V1_TL_S1,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_TL
	{"Reflected L",	BOTH_V1__L_S1,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1__L
	{"Reflected BL",BOTH_V1_BL_S1,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_BL
	{"Reflected B",	BOTH_V1_B__S1,		Q_B,	Q_B,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_B_

	// Broken parries
	{"BParry Top",	BOTH_H1_S1_T_,		Q_T,	Q_B,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_UP,
	{"BParry UR",	BOTH_H1_S1_TR,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_UR,
	{"BParry UL",	BOTH_H1_S1_TL,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_UL,
	{"BParry LR",	BOTH_H1_S1_BL,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LR,
	{"BParry Bot",	BOTH_H1_S1_B_,		Q_B,	Q_T,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LL
	{"BParry LL",	BOTH_H1_S1_BR,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LL

	// Knockaways
	{"Knock Top",	BOTH_K1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_T1_T__BR,		150	},	// LS_PARRY_UP,
	{"Knock UR",	BOTH_K1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_T1_TR__R,		150	},	// LS_PARRY_UR,
	{"Knock UL",	BOTH_K1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_T1_TL__L,		150	},	// LS_PARRY_UL,
	{"Knock LR",	BOTH_K1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_T1_BL_TL,		150	},	// LS_PARRY_LR,
	{"Knock LL",	BOTH_K1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_T1_BR_TR,		150	},	// LS_PARRY_LL

	// Parry
	{"Parry Top",	BOTH_P1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		150	},	// LS_PARRY_UP,
	{"Parry UR",	BOTH_P1_S1_TR,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	// LS_PARRY_UR,
	{"Parry UL",	BOTH_P1_S1_TL,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	// LS_PARRY_UL,
	{"Parry LR",	BOTH_P1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	// LS_PARRY_LR,
	{"Parry LL",	BOTH_P1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	// LS_PARRY_LL

	// Reflecting a missile
	{"Reflect Top",	BOTH_P1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		300	},	// LS_PARRY_UP,
	{"Reflect UR",	BOTH_P1_S1_TL,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_A_TL2BR,		300	},	// LS_PARRY_UR,
	{"Reflect UL",	BOTH_P1_S1_TR,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_TR2BL,		300	},	// LS_PARRY_UL,
	{"Reflect LR",	BOTH_P1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		300	},	// LS_PARRY_LR
	{"Reflect LL",	BOTH_P1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		300	},	// LS_PARRY_LL,
};


saberMoveName_t transitionMove[Q_NUM_QUADS][Q_NUM_QUADS] =
{
	{
		LS_NONE,	//Can't transition to same pos!
		LS_T1_BR__R,//40
		LS_T1_BR_TR,
		LS_T1_BR_T_,
		LS_T1_BR_TL,
		LS_T1_BR__L,
		LS_T1_BR_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__R_BR,//46
		LS_NONE,	//Can't transition to same pos!
		LS_T1__R_TR,
		LS_T1__R_T_,
		LS_T1__R_TL,
		LS_T1__R__L,
		LS_T1__R_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TR_BR,//52
		LS_T1_TR__R,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_TR_T_,
		LS_T1_TR_TL,
		LS_T1_TR__L,
		LS_T1_TR_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_T__BR,//58
		LS_T1_T___R,
		LS_T1_T__TR,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_T__TL,
		LS_T1_T___L,
		LS_T1_T__BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TL_BR,//64
		LS_T1_TL__R,
		LS_T1_TL_TR,
		LS_T1_TL_T_,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_TL__L,
		LS_T1_TL_BL,
		LS_NONE 	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__L_BR,//70
		LS_T1__L__R,
		LS_T1__L_TR,
		LS_T1__L_T_,
		LS_T1__L_TL,
		LS_NONE,	//Can't transition to same pos!
		LS_T1__L_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR,//76
		LS_T1_BL__R,
		LS_T1_BL_TR,
		LS_T1_BL_T_,
		LS_T1_BL_TL,
		LS_T1_BL__L,
		LS_NONE,	//Can't transition to same pos!
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR,//NOTE: there are no transitions from bottom, so re-use the bottom right transitions
		LS_T1_BR__R,
		LS_T1_BR_TR,
		LS_T1_BR_T_,
		LS_T1_BR_TL,
		LS_T1_BR__L,
		LS_T1_BR_BL,
		LS_NONE		//No transitions to bottom, and no anims start there, so shouldn't need any
	}
};

void PM_VelocityForSaberMove( playerState_t *ps, vec3_t throwDir )
{
	vec3_t	vForward = { 0.0f }, vRight = { 0.0f }, vUp = { 0.0f }, startQ = { 0.0f }, endQ = { 0.0f };

	AngleVectors( ps->viewangles, vForward, vRight, vUp );

	switch ( saberMoveData[ps->saberMove].startQuad )
	{
	case Q_BR:
		VectorScale( vRight, 1, startQ );
		VectorMA( startQ, -1, vUp, startQ );
		break;
	case Q_R:
		VectorScale( vRight, 2, startQ );
		break;
	case Q_TR:
		VectorScale( vRight, 1, startQ );
		VectorMA( startQ, 1, vUp, startQ );
		break;
	case Q_T:
		VectorScale( vUp, 2, startQ );
		break;
	case Q_TL:
		VectorScale( vRight, -1, startQ );
		VectorMA( startQ, 1, vUp, startQ );
		break;
	case Q_L:
		VectorScale( vRight, -2, startQ );
		break;
	case Q_BL:
		VectorScale( vRight, -1, startQ );
		VectorMA( startQ, -1, vUp, startQ );
		break;
	case Q_B:
		VectorScale( vUp, -2, startQ );
		break;
	}
	switch ( saberMoveData[ps->saberMove].endQuad )
	{
	case Q_BR:
		VectorScale( vRight, 1, endQ );
		VectorMA( endQ, -1, vUp, endQ );
		break;
	case Q_R:
		VectorScale( vRight, 2, endQ );
		break;
	case Q_TR:
		VectorScale( vRight, 1, endQ );
		VectorMA( endQ, 1, vUp, endQ );
		break;
	case Q_T:
		VectorScale( vUp, 2, endQ );
		break;
	case Q_TL:
		VectorScale( vRight, -1, endQ );
		VectorMA( endQ, 1, vUp, endQ );
		break;
	case Q_L:
		VectorScale( vRight, -2, endQ );
		break;
	case Q_BL:
		VectorScale( vRight, -1, endQ );
		VectorMA( endQ, -1, vUp, endQ );
		break;
	case Q_B:
		VectorScale( vUp, -2, endQ );
		break;
	}
	VectorMA( endQ, 2, vForward, endQ );
	VectorScale( throwDir, 125, throwDir );//FIXME: pass in the throw strength?
	VectorSubtract( endQ, startQ, throwDir );
}

qboolean PM_VelocityForBlockedMove( playerState_t *ps, vec3_t throwDir )
{
	vec3_t	vForward, vRight, vUp;
	AngleVectors( ps->viewangles, vForward, vRight, vUp );
	switch ( ps->saberBlocked )
	{
	case BLOCKED_UPPER_RIGHT:
		VectorScale( vRight, 1, throwDir );
		VectorMA( throwDir, 1, vUp, throwDir );
		break;
	case BLOCKED_UPPER_LEFT:
		VectorScale( vRight, -1, throwDir );
		VectorMA( throwDir, 1, vUp, throwDir );
		break;
	case BLOCKED_LOWER_RIGHT:
		VectorScale( vRight, 1, throwDir );
		VectorMA( throwDir, -1, vUp, throwDir );
		break;
	case BLOCKED_LOWER_LEFT:
		VectorScale( vRight, -1, throwDir );
		VectorMA( throwDir, -1, vUp, throwDir );
		break;
	case BLOCKED_TOP:
		VectorScale( vUp, 2, throwDir );
		break;
	default:
		return qfalse;
		break;
	}
	VectorMA( throwDir, 2, vForward, throwDir );
	VectorScale( throwDir, 250, throwDir );//FIXME: pass in the throw strength?
	return qtrue;
}

int PM_AnimLevelForSaberAnim( int anim )
{
	if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_D1_B____ )
	{
		return FORCE_LEVEL_1;
	}
	if ( anim >= BOTH_A2_T__B_ && anim <= BOTH_D2_B____ )
	{
		return FORCE_LEVEL_2;
	}
	if ( anim >= BOTH_A3_T__B_ && anim <= BOTH_D3_B____ )
	{
		return FORCE_LEVEL_3;
	}
	if ( anim >= BOTH_A4_T__B_ && anim <= BOTH_D4_B____ )
	{//desann
		return FORCE_LEVEL_4;
	}
	if ( anim >= BOTH_A5_T__B_ && anim <= BOTH_D5_B____ )
	{//tavion
		return FORCE_LEVEL_5;
	}
	if ( anim >= BOTH_A6_T__B_ && anim <= BOTH_D6_B____ )
	{//dual
		return SS_DUAL;
	}
	if ( anim >= BOTH_A7_T__B_ && anim <= BOTH_D7_B____ )
	{//staff
		return SS_STAFF;
	}
	return FORCE_LEVEL_0;
}

int PM_PowerLevelForSaberAnim( playerState_t *ps, int saberNum )
{
	int anim = ps->torsoAnim;
	int	animTimeElapsed = PM_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, (animNumber_t)anim ) - ps->torsoAnimTimer;
	if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_D1_B____ )
	{
		//FIXME: these two need their own style
		if ( ps->saber[0].type == SABER_LANCE )
		{
			return FORCE_LEVEL_4;
		}
		else if ( ps->saber[0].type == SABER_TRIDENT )
		{
			return FORCE_LEVEL_3;
		}
		return FORCE_LEVEL_1;
	}
	if ( anim >= BOTH_A2_T__B_ && anim <= BOTH_D2_B____ )
	{
		return FORCE_LEVEL_2;
	}
	if ( anim >= BOTH_A3_T__B_ && anim <= BOTH_D3_B____ )
	{
		return FORCE_LEVEL_3;
	}
	if ( anim >= BOTH_A4_T__B_ && anim <= BOTH_D4_B____ )
	{//desann
		return FORCE_LEVEL_4;
	}
	if ( anim >= BOTH_A5_T__B_ && anim <= BOTH_D5_B____ )
	{//tavion
		return FORCE_LEVEL_2;
	}
	if ( anim >= BOTH_A6_T__B_ && anim <= BOTH_D6_B____ )
	{//dual
		return FORCE_LEVEL_2;
	}
	if ( anim >= BOTH_A7_T__B_ && anim <= BOTH_D7_B____ )
	{//staff
		return FORCE_LEVEL_2;
	}
	if ( ( anim >= BOTH_P1_S1_T_ && anim <= BOTH_P1_S1_BR )
		|| ( anim >= BOTH_P6_S6_T_ && anim <= BOTH_P6_S6_BR )
		|| ( anim >= BOTH_P7_S7_T_ && anim <= BOTH_P7_S7_BR ) )
	{//parries
		switch ( ps->saberAnimLevel )
		{
		case SS_STRONG:
		case SS_DESANN:
			return FORCE_LEVEL_3;
			break;
		case SS_TAVION:
		case SS_STAFF:
		case SS_DUAL:
		case SS_MEDIUM:
			return FORCE_LEVEL_2;
			break;
		case SS_FAST:
			return FORCE_LEVEL_1;
			break;
		default:
			return FORCE_LEVEL_0;
			break;
		}
	}
	if ( ( anim >= BOTH_K1_S1_T_ && anim <= BOTH_K1_S1_BR )
		|| ( anim >= BOTH_K6_S6_T_ && anim <= BOTH_K6_S6_BR )
		|| ( anim >= BOTH_K7_S7_T_ && anim <= BOTH_K7_S7_BR ) )
	{//knockaways
		return FORCE_LEVEL_3;
	}
	if ( ( anim >= BOTH_V1_BR_S1 && anim <= BOTH_V1_B__S1 )
		|| ( anim >= BOTH_V6_BR_S6 && anim <= BOTH_V6_B__S6 )
		|| ( anim >= BOTH_V7_BR_S7 && anim <= BOTH_V7_B__S7 ) )
	{//knocked-away attacks
		return FORCE_LEVEL_1;
	}
	if ( ( anim >= BOTH_H1_S1_T_ && anim <= BOTH_H1_S1_BR )
		|| ( anim >= BOTH_H6_S6_T_ && anim <= BOTH_H6_S6_BR )
		|| ( anim >= BOTH_H7_S7_T_ && anim <= BOTH_H7_S7_BR ) )
	{//broken parries
		return FORCE_LEVEL_0;
	}
	switch ( anim )
	{
	case BOTH_A2_STABBACK1:
		if ( ps->torsoAnimTimer < 450 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 400 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_ATTACK_BACK:
		if ( ps->torsoAnimTimer < 500 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_CROUCHATTACKBACK1:
		if ( ps->torsoAnimTimer < 800 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
		//FIXME: break up?
		return FORCE_LEVEL_3;
		break;
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
		//FIXME: break up?
		return FORCE_LEVEL_3;
		break;
	case BOTH_K1_S1_T_:	//# knockaway saber top
	case BOTH_K1_S1_TR:	//# knockaway saber top right
	case BOTH_K1_S1_TL:	//# knockaway saber top left
	case BOTH_K1_S1_BL:	//# knockaway saber bottom left
	case BOTH_K1_S1_B_:	//# knockaway saber bottom
	case BOTH_K1_S1_BR:	//# knockaway saber bottom right
		//FIXME: break up?
		return FORCE_LEVEL_3;
		break;
	case BOTH_LUNGE2_B__T_:
		if ( ps->torsoAnimTimer < 400 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 150 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_FORCELEAP2_T__B_:
		if ( ps->torsoAnimTimer < 400 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 550 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
		return FORCE_LEVEL_3;//???
		break;
	case BOTH_JUMPFLIPSLASHDOWN1:
		if ( ps->torsoAnimTimer <= 900 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 550 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_JUMPFLIPSTABDOWN:
		if ( ps->torsoAnimTimer <= 1200 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed <= 250 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_JUMPATTACK6:
		/*
		if (pm->ps)
		{
			if ( ( pm->ps->legsAnimTimer >= 1450
					&& PM_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, BOTH_JUMPATTACK6 ) - pm->ps->legsAnimTimer >= 400 )
				||(pm->ps->legsAnimTimer >= 400
					&& PM_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, BOTH_JUMPATTACK6 ) - pm->ps->legsAnimTimer >= 1100 ) )
			{//pretty much sideways
				return FORCE_LEVEL_3;
			}
		}
		*/
		if ( ( ps->torsoAnimTimer >= 1450
				&& animTimeElapsed >= 400 )
			||(ps->torsoAnimTimer >= 400
				&& animTimeElapsed >= 1100 ) )
		{//pretty much sideways
			return FORCE_LEVEL_3;
		}
		return FORCE_LEVEL_0;
		break;
	case BOTH_JUMPATTACK7:
		if ( ps->torsoAnimTimer <= 1200 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_SPINATTACK6:
		if ( animTimeElapsed <= 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_SPINATTACK7:
		if ( ps->torsoAnimTimer <= 500 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 500 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_FORCELONGLEAP_ATTACK:
		if ( animTimeElapsed <= 200 )
		{//1st four frames of anim
			return FORCE_LEVEL_3;
		}
		break;
	/*
	case BOTH_A7_KICK_F://these kicks attack, too
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
		//FIXME: break up
		return FORCE_LEVEL_3;
		break;
	*/
	case BOTH_STABDOWN:
		if ( ps->torsoAnimTimer <= 900 )
		{//end of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_STABDOWN_STAFF:
		if ( ps->torsoAnimTimer <= 850 )
		{//end of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_STABDOWN_DUAL:
		if ( ps->torsoAnimTimer <= 900 )
		{//end of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_A6_SABERPROTECT:
		if ( ps->torsoAnimTimer < 650 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A7_SOULCAL:
		if ( ps->torsoAnimTimer < 650 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 600 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A1_SPECIAL:
		if ( ps->torsoAnimTimer < 600 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A2_SPECIAL:
		if ( ps->torsoAnimTimer < 300 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A3_SPECIAL:
		if ( ps->torsoAnimTimer < 700 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_FLIP_ATTACK7:
		return FORCE_LEVEL_3;
		break;
	case BOTH_PULL_IMPALE_STAB:
		if ( ps->torsoAnimTimer < 1000 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_PULL_IMPALE_SWING:
		if ( ps->torsoAnimTimer < 500 )//750 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 650 )//600 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_ALORA_SPIN_SLASH:
		if ( ps->torsoAnimTimer < 900 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 250 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A6_FB:
		if ( ps->torsoAnimTimer < 250 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 250 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A6_LR:
		if ( ps->torsoAnimTimer < 250 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 250 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A7_HILT:
		return FORCE_LEVEL_0;
		break;
//===SABERLOCK SUPERBREAKS START===========================================================================
	case BOTH_LK_S_DL_T_SB_1_W:
		if ( ps->torsoAnimTimer < 700 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_S_ST_S_SB_1_W:
		if ( ps->torsoAnimTimer < 300 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
		if ( ps->torsoAnimTimer < 700 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 400 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
		if ( ps->torsoAnimTimer < 150 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 400 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_DL_T_SB_1_W:
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
		if ( animTimeElapsed < 1000 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_ST_T_SB_1_W:
		if ( ps->torsoAnimTimer < 950 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 650 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_S_S_SB_1_W:
		if ( saberNum != 0 )
		{//only right hand saber does damage in this suberbreak
			return FORCE_LEVEL_0;
		}
		if ( ps->torsoAnimTimer < 900 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 450 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_S_T_SB_1_W:
		if ( saberNum != 0 )
		{//only right hand saber does damage in this suberbreak
			return FORCE_LEVEL_0;
		}
		if ( ps->torsoAnimTimer < 250 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 150 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_ST_DL_S_SB_1_W:
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_ST_DL_T_SB_1_W:
		//special suberbreak - doesn't kill, just kicks them backwards
		return FORCE_LEVEL_0;
		break;
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
		if ( ps->torsoAnimTimer < 800 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 350 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
		return FORCE_LEVEL_5;
		break;
//===SABERLOCK SUPERBREAKS START===========================================================================
	case BOTH_HANG_ATTACK:
		//FIME: break up
		if ( ps->torsoAnimTimer < 1000 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( animTimeElapsed < 250 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		else
		{//sweet spot
			return FORCE_LEVEL_5;
		}
		break;
	case BOTH_ROLL_STAB:
		if ( animTimeElapsed > 400 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else
		{
			return FORCE_LEVEL_3;
		}
		break;
	}
	return FORCE_LEVEL_0;
}

qboolean PM_InAnimForSaberMove( int anim, int saberMove )
{
	switch ( anim )
	{//special case anims
	case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_DL_T_SB_1_W:
	case BOTH_LK_S_ST_S_SB_1_W:
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_DL_T_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
	case BOTH_LK_DL_ST_T_SB_1_W:
	case BOTH_LK_DL_S_S_SB_1_W:
	case BOTH_LK_DL_S_T_SB_1_W:
	case BOTH_LK_ST_DL_S_SB_1_W:
	case BOTH_LK_ST_DL_T_SB_1_W:
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
	case BOTH_HANG_ATTACK:
		return qtrue;
	}
	if ( PM_SaberDrawPutawayAnim( anim ) )
	{
		if ( saberMove == LS_DRAW || saberMove == LS_PUTAWAY )
		{
			return qtrue;
		}
		return qfalse;
	}
	else if ( PM_SaberStanceAnim( anim ) )
	{
		if ( saberMove == LS_READY )
		{
			return qtrue;
		}
		return qfalse;
	}
	int animLevel = PM_AnimLevelForSaberAnim( anim );
	if ( animLevel <= 0 )
	{//NOTE: this will always return false for the ready poses and putaway/draw...
		return qfalse;
	}
	//drop the anim to the first level and start the checks there
	anim -= (animLevel-FORCE_LEVEL_1)*SABER_ANIM_GROUP_SIZE;
	//check level 1
	if ( anim == saberMoveData[saberMove].animToUse )
	{
		return qtrue;
	}
	//check level 2
	anim += SABER_ANIM_GROUP_SIZE;
	if ( anim == saberMoveData[saberMove].animToUse )
	{
		return qtrue;
	}
	//check level 3
	anim += SABER_ANIM_GROUP_SIZE;
	if ( anim == saberMoveData[saberMove].animToUse )
	{
		return qtrue;
	}
	//check level 4
	anim += SABER_ANIM_GROUP_SIZE;
	if ( anim == saberMoveData[saberMove].animToUse )
	{
		return qtrue;
	}
	//check level 5
	anim += SABER_ANIM_GROUP_SIZE;
	if ( anim == saberMoveData[saberMove].animToUse )
	{
		return qtrue;
	}
	if ( anim >= BOTH_P1_S1_T_ && anim <= BOTH_H1_S1_BR )
	{//parries, knockaways and broken parries
		return (anim==saberMoveData[saberMove].animToUse);
	}
	return qfalse;
}

qboolean PM_SaberInIdle( int move )
{
	switch ( move )
	{
	case LS_NONE:
	case LS_READY:
	case LS_DRAW:
	case LS_PUTAWAY:
		return qtrue;
		break;
	}
	return qfalse;
}
qboolean PM_SaberInSpecialAttack( int anim )
{
	switch ( anim )
	{
	case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_DL_T_SB_1_W:
	case BOTH_LK_S_ST_S_SB_1_W:
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_DL_T_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
	case BOTH_LK_DL_ST_T_SB_1_W:
	case BOTH_LK_DL_S_S_SB_1_W:
	case BOTH_LK_DL_S_T_SB_1_W:
	case BOTH_LK_ST_DL_S_SB_1_W:
	case BOTH_LK_ST_DL_T_SB_1_W:
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
	case BOTH_HANG_ATTACK:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInAttackPure( int move )
{
	if ( move >= LS_A_TL2BR && move <= LS_A_T2B )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInAttack( int move )
{
	if ( move >= LS_A_TL2BR && move <= LS_A_T2B )
	{
		return qtrue;
	}
	switch ( move )
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_KICK_F:
	case LS_KICK_B:
	case LS_KICK_R:
	case LS_KICK_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_STABDOWN:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
		break;
	}
	return qfalse;
}
qboolean PM_SaberInTransition( int move )
{
	if ( move >= LS_T1_BR__R && move <= LS_T1_BL__L )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInStart( int move )
{
	if ( move >= LS_S_TL2BR && move <= LS_S_T2B )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInReturn( int move )
{
	if ( move >= LS_R_TL2BR && move <= LS_R_T2B )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInTransitionAny( int move )
{
	if ( PM_SaberInStart( move ) )
	{
		return qtrue;
	}
	else if ( PM_SaberInTransition( move ) )
	{
		return qtrue;
	}
	else if ( PM_SaberInReturn( move ) )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInBounce( int move )
{
	if ( move >= LS_B1_BR && move <= LS_B1_BL )
	{
		return qtrue;
	}
	if ( move >= LS_D1_BR && move <= LS_D1_BL )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInBrokenParry( int move )
{
	if ( move >= LS_V1_BR && move <= LS_V1_B_ )
	{
		return qtrue;
	}
	if ( move >= LS_H1_T_ && move <= LS_H1_BL )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInDeflect( int move )
{
	if ( move >= LS_D1_BR && move <= LS_D1_B_ )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInParry( int move )
{
	if ( move >= LS_PARRY_UP && move <= LS_PARRY_LL )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInKnockaway( int move )
{
	if ( move >= LS_K1_T_ && move <= LS_K1_BL )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean PM_SaberInReflect( int move )
{
	if ( move >= LS_REFLECT_UP && move <= LS_REFLECT_LL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInSpecial( int move )
{
	switch( move )
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_KICK_F:
	case LS_KICK_B:
	case LS_KICK_R:
	case LS_KICK_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_STABDOWN:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_KickMove( int move )
{
	switch( move )
	{
	case LS_KICK_F:
	case LS_KICK_B:
	case LS_KICK_R:
	case LS_KICK_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_HILT_BASH:
	case LS_KICK_F_AIR:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberCanInterruptMove( int move, int anim )
{
	if ( PM_InAnimForSaberMove( anim, move ) )
	{
		switch( move )
		{
		case LS_A_BACK:
		case LS_A_BACK_CR:
		case LS_A_BACKSTAB:
		case LS_ROLL_STAB:
		case LS_A_LUNGE:
		case LS_A_JUMP_T__B_:
		case LS_A_FLIP_STAB:
		case LS_A_FLIP_SLASH:
		case LS_JUMPATTACK_DUAL:
		case LS_JUMPATTACK_CART_LEFT:
		case LS_JUMPATTACK_CART_RIGHT:
		case LS_JUMPATTACK_STAFF_LEFT:
		case LS_JUMPATTACK_STAFF_RIGHT:
		case LS_BUTTERFLY_LEFT:
		case LS_BUTTERFLY_RIGHT:
		case LS_A_BACKFLIP_ATK:
		case LS_SPINATTACK_DUAL:
		case LS_SPINATTACK:
		case LS_LEAP_ATTACK:
		case LS_SWOOP_ATTACK_RIGHT:
		case LS_SWOOP_ATTACK_LEFT:
		case LS_TAUNTAUN_ATTACK_RIGHT:
		case LS_TAUNTAUN_ATTACK_LEFT:
		case LS_KICK_S:
		case LS_KICK_BF:
		case LS_KICK_RL:
		case LS_STABDOWN:
		case LS_STABDOWN_STAFF:
		case LS_STABDOWN_DUAL:
		case LS_DUAL_SPIN_PROTECT:
		case LS_STAFF_SOULCAL:
		case LS_A1_SPECIAL:
		case LS_A2_SPECIAL:
		case LS_A3_SPECIAL:
		case LS_UPSIDE_DOWN_ATTACK:
		case LS_PULL_ATTACK_STAB:
		case LS_PULL_ATTACK_SWING:
		case LS_SPINATTACK_ALORA:
		case LS_DUAL_FB:
		case LS_DUAL_LR:
		case LS_HILT_BASH:
			return qfalse;
		}

		if ( PM_SaberInAttackPure( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInStart( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInTransition( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInBounce( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInBrokenParry( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInDeflect( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInParry( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInKnockaway( move ) )
		{
			return qfalse;
		}
		if ( PM_SaberInReflect( move ) )
		{
			return qfalse;
		}
	}
	switch ( anim )
	{
	case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_DL_T_SB_1_W:
	case BOTH_LK_S_ST_S_SB_1_W:
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_DL_T_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
	case BOTH_LK_DL_ST_T_SB_1_W:
	case BOTH_LK_DL_S_S_SB_1_W:
	case BOTH_LK_DL_S_T_SB_1_W:
	case BOTH_LK_ST_DL_S_SB_1_W:
	case BOTH_LK_ST_DL_T_SB_1_W:
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
	case BOTH_HANG_ATTACK:
		return qfalse;
	}
	return qtrue;
}

saberMoveName_t PM_BrokenParryForAttack( int move )
{
	//Our attack was knocked away by a knockaway parry
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( saberMoveData[move].startQuad )
	{
	case Q_B:
		return LS_V1_B_;
		break;
	case Q_BR:
		return LS_V1_BR;
		break;
	case Q_R:
		return LS_V1__R;
		break;
	case Q_TR:
		return LS_V1_TR;
		break;
	case Q_T:
		return LS_V1_T_;
		break;
	case Q_TL:
		return LS_V1_TL;
		break;
	case Q_L:
		return LS_V1__L;
		break;
	case Q_BL:
		return LS_V1_BL;
		break;
	}
	return LS_NONE;
}

saberMoveName_t PM_BrokenParryForParry( int move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case LS_PARRY_UP:
		//Hmm... since we don't know what dir the hit came from, randomly pick knock down or knock back
		if ( Q_irand( 0, 1 ) )
		{
			return LS_H1_B_;
		}
		else
		{
			return LS_H1_T_;
		}
		break;
	case LS_PARRY_UR:
		return LS_H1_TR;
		break;
	case LS_PARRY_UL:
		return LS_H1_TL;
		break;
	case LS_PARRY_LR:
		return LS_H1_BR;
		break;
	case LS_PARRY_LL:
		return LS_H1_BL;
		break;
	case LS_READY:
		return LS_H1_B_;//???
		break;
	}
	return LS_NONE;
}

saberMoveName_t PM_KnockawayForParry( int move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case BLOCKED_TOP://LS_PARRY_UP:
		return LS_K1_T_;//push up
		break;
	case BLOCKED_UPPER_RIGHT://LS_PARRY_UR:
	default://case LS_READY:
		return LS_K1_TR;//push up, slightly to right
		break;
	case BLOCKED_UPPER_LEFT://LS_PARRY_UL:
		return LS_K1_TL;//push up and to left
		break;
	case BLOCKED_LOWER_RIGHT://LS_PARRY_LR:
		return LS_K1_BR;//push down and to left
		break;
	case BLOCKED_LOWER_LEFT://LS_PARRY_LL:
		return LS_K1_BL;//push down and to right
		break;
	}
	//return LS_NONE;
}

saberMoveName_t PM_SaberBounceForAttack( int move )
{
	switch ( saberMoveData[move].startQuad )
	{
	case Q_B:
	case Q_BR:
		return LS_B1_BR;
		break;
	case Q_R:
		return LS_B1__R;
		break;
	case Q_TR:
		return LS_B1_TR;
		break;
	case Q_T:
		return LS_B1_T_;
		break;
	case Q_TL:
		return LS_B1_TL;
		break;
	case Q_L:
		return LS_B1__L;
		break;
	case Q_BL:
		return LS_B1_BL;
		break;
	}
	return LS_NONE;
}

saberMoveName_t PM_AttackMoveForQuad( int quad )
{
	switch ( quad )
	{
	case Q_B:
	case Q_BR:
		return LS_A_BR2TL;
		break;
	case Q_R:
		return LS_A_R2L;
		break;
	case Q_TR:
		return LS_A_TR2BL;
		break;
	case Q_T:
		return LS_A_T2B;
		break;
	case Q_TL:
		return LS_A_TL2BR;
		break;
	case Q_L:
		return LS_A_L2R;
		break;
	case Q_BL:
		return LS_A_BL2TR;
		break;
	}
	return LS_NONE;
}

int saberMoveTransitionAngle[Q_NUM_QUADS][Q_NUM_QUADS] =
{
	{
		0,//Q_BR,Q_BR,
		45,//Q_BR,Q_R,
		90,//Q_BR,Q_TR,
		135,//Q_BR,Q_T,
		180,//Q_BR,Q_TL,
		215,//Q_BR,Q_L,
		270,//Q_BR,Q_BL,
		45,//Q_BR,Q_B,
	},
	{
		45,//Q_R,Q_BR,
		0,//Q_R,Q_R,
		45,//Q_R,Q_TR,
		90,//Q_R,Q_T,
		135,//Q_R,Q_TL,
		180,//Q_R,Q_L,
		215,//Q_R,Q_BL,
		90,//Q_R,Q_B,
	},
	{
		90,//Q_TR,Q_BR,
		45,//Q_TR,Q_R,
		0,//Q_TR,Q_TR,
		45,//Q_TR,Q_T,
		90,//Q_TR,Q_TL,
		135,//Q_TR,Q_L,
		180,//Q_TR,Q_BL,
		135,//Q_TR,Q_B,
	},
	{
		135,//Q_T,Q_BR,
		90,//Q_T,Q_R,
		45,//Q_T,Q_TR,
		0,//Q_T,Q_T,
		45,//Q_T,Q_TL,
		90,//Q_T,Q_L,
		135,//Q_T,Q_BL,
		180,//Q_T,Q_B,
	},
	{
		180,//Q_TL,Q_BR,
		135,//Q_TL,Q_R,
		90,//Q_TL,Q_TR,
		45,//Q_TL,Q_T,
		0,//Q_TL,Q_TL,
		45,//Q_TL,Q_L,
		90,//Q_TL,Q_BL,
		135,//Q_TL,Q_B,
	},
	{
		215,//Q_L,Q_BR,
		180,//Q_L,Q_R,
		135,//Q_L,Q_TR,
		90,//Q_L,Q_T,
		45,//Q_L,Q_TL,
		0,//Q_L,Q_L,
		45,//Q_L,Q_BL,
		90,//Q_L,Q_B,
	},
	{
		270,//Q_BL,Q_BR,
		215,//Q_BL,Q_R,
		180,//Q_BL,Q_TR,
		135,//Q_BL,Q_T,
		90,//Q_BL,Q_TL,
		45,//Q_BL,Q_L,
		0,//Q_BL,Q_BL,
		45,//Q_BL,Q_B,
	},
	{
		45,//Q_B,Q_BR,
		90,//Q_B,Q_R,
		135,//Q_B,Q_TR,
		180,//Q_B,Q_T,
		135,//Q_B,Q_TL,
		90,//Q_B,Q_L,
		45,//Q_B,Q_BL,
		0//Q_B,Q_B,
	}
};

int PM_SaberAttackChainAngle( int move1, int move2 )
{
	if ( move1 == -1 || move2 == -1 )
	{
		return -1;
	}
	return saberMoveTransitionAngle[saberMoveData[move1].endQuad][saberMoveData[move2].startQuad];
}

qboolean PM_SaberKataDone( int curmove = LS_NONE, int newmove = LS_NONE )
{
	if ( pm->ps->forceRageRecoveryTime > level.time )
	{//rage recovery, only 1 swing at a time (tired)
		if ( pm->ps->saberAttackChainCount > 0 )
		{//swung once
			return qtrue;
		}
		else
		{//allow one attack
			return qfalse;
		}
	}
	else if ( (pm->ps->forcePowersActive&(1<<FP_RAGE)) )
	{//infinite chaining when raged
		return qfalse;
	}
	else if ( pm->ps->saber[0].maxChain == -1 )
	{
		return qfalse;
	}
	else if ( pm->ps->saber[0].maxChain != 0 )
	{
		if ( pm->ps->saberAttackChainCount >= pm->ps->saber[0].maxChain )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	if ( pm->ps->saberAnimLevel == SS_DESANN || pm->ps->saberAnimLevel == SS_TAVION )
	{//desann and tavion can link up as many attacks as they want
		return qfalse;
	}
	//FIXME: instead of random, apply some sort of logical conditions to whether or
	//		not you can chain?  Like if you were completely missed, you can't chain as much, or...?
	//		And/Or based on FP_SABER_OFFENSE level?  So number of attacks you can chain
	//		increases with your FP_SABER_OFFENSE skill?
	if ( pm->ps->saberAnimLevel == SS_STAFF )
	{
		//TEMP: for now, let staff attacks infinitely chain
		return qfalse;
		/*
		if ( pm->ps->saberAttackChainCount > Q_irand( 3, 4 ) )
		{
			return qtrue;
		}
		else if ( pm->ps->saberAttackChainCount > 0 )
		{
			int chainAngle = PM_SaberAttackChainAngle( curmove, newmove );
			if ( chainAngle < 135 || chainAngle > 215 )
			{//if trying to chain to a move that doesn't continue the momentum
				if ( pm->ps->saberAttackChainCount > 1 )
				{
					return qtrue;
				}
			}
			else if ( chainAngle == 180 )
			{//continues the momentum perfectly, allow it to chain 66% of the time
				if ( pm->ps->saberAttackChainCount > 2 )
				{
					return qtrue;
				}
			}
			else
			{//would continue the movement somewhat, 50% chance of continuing
				if ( pm->ps->saberAttackChainCount > 3 )
				{
					return qtrue;
				}
			}
		}
		*/
	}
	else if ( pm->ps->saberAnimLevel == SS_DUAL )
	{
		//TEMP: for now, let staff attacks infinitely chain
		return qfalse;
	}
	else if ( pm->ps->saberAnimLevel == FORCE_LEVEL_3 )
	{
		if ( curmove == LS_NONE || newmove == LS_NONE )
		{
			if ( pm->ps->saberAnimLevel >= FORCE_LEVEL_3 && pm->ps->saberAttackChainCount > Q_irand( 0, 1 ) )
			{
				return qtrue;
			}
		}
		else if ( pm->ps->saberAttackChainCount > Q_irand( 2, 3 ) )
		{
			return qtrue;
		}
		else if ( pm->ps->saberAttackChainCount > 0 )
		{
			int chainAngle = PM_SaberAttackChainAngle( curmove, newmove );
			if ( chainAngle < 135 || chainAngle > 215 )
			{//if trying to chain to a move that doesn't continue the momentum
				return qtrue;
			}
			else if ( chainAngle == 180 )
			{//continues the momentum perfectly, allow it to chain 66% of the time
				if ( pm->ps->saberAttackChainCount > 1 )
				{
					return qtrue;
				}
			}
			else
			{//would continue the movement somewhat, 50% chance of continuing
				if ( pm->ps->saberAttackChainCount > 2 )
				{
					return qtrue;
				}
			}
		}
	}
	else
	{//FIXME: have chainAngle influence fast and medium chains as well?
		if ( (pm->ps->saberAnimLevel == FORCE_LEVEL_2 || pm->ps->saberAnimLevel == SS_DUAL)
			&& pm->ps->saberAttackChainCount > Q_irand( 2, 5 ) )
		{
			return qtrue;
		}
	}
	return qfalse;
}

qboolean PM_CheckEnemyInBack( float backCheckDist )
{
	if ( !pm->gent || !pm->gent->client )
	{
		return qfalse;
	}
	if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer())
		&& !g_saberAutoAim->integer && pm->cmd.forwardmove >= 0 )
	{//don't auto-backstab
		return qfalse;
	}
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{//only when on ground
		return qfalse;
	}
	trace_t	trace;
	vec3_t end, fwd, fwdAngles = {0,pm->ps->viewangles[YAW],0};

	AngleVectors( fwdAngles, fwd, NULL, NULL );
	VectorMA( pm->ps->origin, -backCheckDist, fwd, end );

	pm->trace( &trace, pm->ps->origin, vec3_origin, vec3_origin, end, pm->ps->clientNum, CONTENTS_SOLID|CONTENTS_BODY, (EG2_Collision)0, 0 );
	if ( trace.fraction < 1.0f && trace.entityNum < ENTITYNUM_WORLD )
	{
		gentity_t *traceEnt = &g_entities[trace.entityNum];
		if ( traceEnt
			&& traceEnt->health > 0
			&& traceEnt->client
			&& traceEnt->client->playerTeam == pm->gent->client->enemyTeam
			&& traceEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) )
			{//player
				if ( pm->gent )
				{//set player enemy to traceEnt so he auto-aims at him
					pm->gent->enemy = traceEnt;
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

saberMoveName_t PM_PickBackStab( void )
{
	if ( !pm->gent || !pm->gent->client )
	{
		return LS_READY;
	}
	if ( pm->ps->dualSabers
		&& pm->ps->saber[1].Active() )
	{
		if ( pm->ps->pm_flags & PMF_DUCKED )
		{
			return LS_A_BACK_CR;
		}
		else
		{
			return LS_A_BACK;
		}
	}
	if ( pm->gent->client->ps.saberAnimLevel == SS_TAVION )
	{
		return LS_A_BACKSTAB;
	}
	else if ( pm->gent->client->ps.saberAnimLevel == SS_DESANN )
	{
		if ( pm->ps->saberMove == LS_READY || !Q_irand( 0, 3 ) )
		{
			return LS_A_BACKSTAB;
		}
		else if ( pm->ps->pm_flags & PMF_DUCKED )
		{
			return LS_A_BACK_CR;
		}
		else
		{
			return LS_A_BACK;
		}
	}
	else if ( pm->ps->saberAnimLevel == FORCE_LEVEL_2
		|| pm->ps->saberAnimLevel == SS_DUAL )
	{//using medium attacks or dual sabers
		if ( pm->ps->pm_flags & PMF_DUCKED )
		{
			return LS_A_BACK_CR;
		}
		else
		{
			return LS_A_BACK;
		}
	}
	else
	{
		return LS_A_BACKSTAB;
	}
}

saberMoveName_t PM_CheckStabDown( void )
{
	if ( !pm->gent || !pm->gent->enemy || !pm->gent->enemy->client )
	{
		return LS_NONE;
	}
	if ( (pm->ps->saber[0].saberFlags&SFL_NO_STABDOWN) )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers
		&& (pm->ps->saber[1].saberFlags&SFL_NO_STABDOWN) )
	{
		return LS_NONE;
	}
	if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
	{//player
		if ( G_TryingKataAttack( pm->gent, &pm->cmd ) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
		{//want to try a special
			return LS_NONE;
		}
	}
	if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) )
	{//player
		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )//in air
		{//sorry must be on ground (or have just jumped)
			if ( level.time-pm->ps->lastOnGround <= 50 && (pm->ps->pm_flags&PMF_JUMPING) )
			{//just jumped, it's okay
			}
			else
			{
				return LS_NONE;
			}
		}
		/*
		if ( pm->cmd.upmove > 0 )
		{//trying to jump
		}
		else if ( pm->ps->groundEntityNum == ENTITYNUM_NONE //in air
			&& level.time-pm->ps->lastOnGround <= 250 //just left ground
			&& (pm->ps->pm_flags&PMF_JUMPING) )//jumped
		{//just jumped
		}
		else
		{
			return LS_NONE;
		}
		*/
		pm->ps->velocity[2] = 0;
		pm->cmd.upmove = 0;
	}
	else if ( (pm->ps->clientNum >= MAX_CLIENTS&&!PM_ControlledByPlayer()) )
	{//NPC
		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )//in air
		{//sorry must be on ground (or have just jumped)
			if ( level.time-pm->ps->lastOnGround <= 250 && (pm->ps->pm_flags&PMF_JUMPING) )
			{//just jumped, it's okay
			}
			else
			{
				return LS_NONE;
			}
		}
		if ( !pm->gent->NPC )
		{//wtf???
			return LS_NONE;
		}
		if ( Q_irand( 0, RANK_CAPTAIN ) > pm->gent->NPC->rank )
		{//lower ranks do this less often
			return LS_NONE;
		}
	}
	vec3_t enemyDir, faceFwd, facingAngles = {0, pm->ps->viewangles[YAW], 0};
	AngleVectors( facingAngles, faceFwd, NULL, NULL );
	VectorSubtract( pm->gent->enemy->currentOrigin, pm->ps->origin, enemyDir );
	float enemyZDiff = enemyDir[2];
	enemyDir[2] = 0;
	float enemyHDist = VectorNormalize( enemyDir )-(pm->gent->maxs[0]+pm->gent->enemy->maxs[0]);
	float dot = DotProduct( enemyDir, faceFwd );

	if ( //(pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer())
		dot > 0.65f
		//&& enemyHDist >= 32 //was 48
		&& enemyHDist <= 164//was 112
		&& PM_InKnockDownOnGround( &pm->gent->enemy->client->ps )//still on ground
		&& !PM_InGetUpNoRoll( &pm->gent->enemy->client->ps )//not getting up yet
		&& enemyZDiff <= 20 )
	{//guy is on the ground below me, do a top-down attack
		if ( pm->gent->enemy->s.number >= MAX_CLIENTS
			|| !G_ControlledByPlayer( pm->gent->enemy ) )
		{//don't get up while I'm doing this
			//stop them from trying to get up for at least another 3 seconds
			TIMER_Set( pm->gent->enemy, "noGetUpStraight", 3000 );
		}
		//pick the right anim
		if ( pm->ps->saberAnimLevel == SS_DUAL
			|| (pm->ps->dualSabers&&pm->ps->saber[1].Active()) )
		{
			return LS_STABDOWN_DUAL;
		}
		else if ( pm->ps->saberAnimLevel == SS_STAFF )
		{
			return LS_STABDOWN_STAFF;
		}
		else
		{
			return LS_STABDOWN;
		}
	}
	return LS_NONE;
}

extern saberMoveName_t PM_NPCSaberAttackFromQuad( int quad );
saberMoveName_t PM_SaberFlipOverAttackMove( void );
saberMoveName_t PM_AttackForEnemyPos( qboolean allowFB, qboolean allowStabDown )
{
	saberMoveName_t autoMove = LS_INVALID;

	if( !pm->gent->enemy )
	{
		return LS_NONE;
	}

	vec3_t enemy_org, enemyDir, faceFwd, faceRight, faceUp, facingAngles = {0, pm->ps->viewangles[YAW], 0};
	AngleVectors( facingAngles, faceFwd, faceRight, faceUp );
	//FIXME: predict enemy position?
	if ( pm->gent->enemy->client )
	{
		//VectorCopy( pm->gent->enemy->currentOrigin, enemy_org );
		//HMM... using this will adjust for bbox size, so let's do that...
		vec3_t	size;
		VectorSubtract( pm->gent->enemy->absmax, pm->gent->enemy->absmin, size );
		VectorMA( pm->gent->enemy->absmin, 0.5, size, enemy_org );

		VectorSubtract( pm->gent->enemy->client->renderInfo.eyePoint, pm->ps->origin, enemyDir );
	}
	else
	{
		if ( pm->gent->enemy->bmodel && VectorCompare( vec3_origin, pm->gent->enemy->currentOrigin ) )
		{//a brush model without an origin brush
			vec3_t	size;
			VectorSubtract( pm->gent->enemy->absmax, pm->gent->enemy->absmin, size );
			VectorMA( pm->gent->enemy->absmin, 0.5, size, enemy_org );
		}
		else
		{
			VectorCopy( pm->gent->enemy->currentOrigin, enemy_org );
		}
		VectorSubtract( enemy_org, pm->ps->origin, enemyDir );
	}
	float enemyZDiff = enemyDir[2];
	float enemyDist = VectorNormalize( enemyDir );
	float dot = DotProduct( enemyDir, faceFwd );
	if ( dot > 0 )
	{//enemy is in front
		if ( allowStabDown )
		{//okay to try this
			saberMoveName_t stabDownMove = PM_CheckStabDown();
			if ( stabDownMove != LS_NONE )
			{
				return stabDownMove;
			}
		}
		if ( (pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer())
			&& dot > 0.65f
			&& enemyDist <= 64 && pm->gent->enemy->client
			&& (enemyZDiff <= 20 || PM_InKnockDownOnGround( &pm->gent->enemy->client->ps ) || PM_CrouchAnim( pm->gent->enemy->client->ps.legsAnim ) ) )
		{//swing down at them
			return LS_A_T2B;
		}
		if ( allowFB )
		{//directly in front anim allowed
			if ( !(pm->ps->saber[0].saberFlags&SFL_NO_BACK_ATTACK)
				&& (!pm->ps->dualSabers || !(pm->ps->saber[1].saberFlags&SFL_NO_BACK_ATTACK)) )
			{//okay to do backstabs with this saber
				if ( enemyDist > 200 || pm->gent->enemy->health <= 0 )
				{//hmm, look in back for an enemy
					if ( pm->ps->clientNum && !PM_ControlledByPlayer() )
					{//player should never do this automatically
						if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
						{//only when on ground
							if ( pm->gent && pm->gent->client && pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG && Q_irand( 0, pm->gent->NPC->rank ) > RANK_ENSIGN )
							{//only fencers and higher can do this, higher rank does it more
								if ( PM_CheckEnemyInBack( 100 ) )
								{
									return PM_PickBackStab();
								}
							}
						}
					}
				}
			}
			//this is the default only if they're *right* in front...
			if ( (pm->ps->clientNum&&!PM_ControlledByPlayer())
				|| ((pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
			{//NPC or player not in 1st person
				if ( PM_CheckFlipOverAttackMove( qtrue ) )
				{//enemy must be close and in front
					return PM_SaberFlipOverAttackMove();
				}
			}
			if ( PM_CheckLungeAttackMove() )
			{//NPC
				autoMove = PM_SaberLungeAttackMove( qtrue );
			}
			else
			{
				autoMove = LS_A_T2B;
			}
		}
		else
		{//pick a random one
			if ( Q_irand( 0, 1 ) )
			{
				autoMove = LS_A_TR2BL;
			}
			else
			{
				autoMove = LS_A_TL2BR;
			}
		}
		float dotR = DotProduct( enemyDir, faceRight );
		if ( dotR > 0.35 )
		{//enemy is to far right
			autoMove = LS_A_L2R;
		}
		else if ( dotR < -0.35 )
		{//far left
			autoMove = LS_A_R2L;
		}
		else if ( dotR > 0.15 )
		{//enemy is to near right
			autoMove = LS_A_TR2BL;
		}
		else if ( dotR < -0.15 )
		{//near left
			autoMove = LS_A_TL2BR;
		}
		if ( DotProduct( enemyDir, faceUp ) > 0.5 )
		{//enemy is above me
			if ( autoMove == LS_A_TR2BL )
			{
				autoMove = LS_A_BL2TR;
			}
			else if ( autoMove == LS_A_TL2BR )
			{
				autoMove = LS_A_BR2TL;
			}
		}
	}
	else if ( allowFB )
	{//back attack allowed
		//if ( !PM_InKnockDown( pm->ps ) )
		if ( !(pm->ps->saber[0].saberFlags&SFL_NO_BACK_ATTACK)
			&& (!pm->ps->dualSabers || !(pm->ps->saber[1].saberFlags&SFL_NO_BACK_ATTACK)) )
		{//okay to do backstabs with this saber
			if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
			{//only when on ground
				if ( !pm->gent->enemy->client || pm->gent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//enemy not a client or is a client and on ground
					if ( dot < -0.75f
						&& enemyDist < 128
						&& (pm->ps->saberAnimLevel == SS_FAST || pm->ps->saberAnimLevel == SS_STAFF || (pm->gent->client &&(pm->gent->client->NPC_class == CLASS_TAVION||pm->gent->client->NPC_class == CLASS_ALORA)&&Q_irand(0,2))) )
					{//fast back-stab
						if ( !(pm->ps->pm_flags&PMF_DUCKED) && pm->cmd.upmove >= 0 )
						{//can't do it while ducked?
							if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) || (pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG) )
							{//only fencers and above can do this
								autoMove = LS_A_BACKSTAB;
							}
						}
					}
					else if ( pm->ps->saberAnimLevel != SS_FAST
						&& pm->ps->saberAnimLevel != SS_STAFF )
					{//higher level back spin-attacks
						if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) || ((pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
						{
							if ( (pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0 )
							{
								autoMove = LS_A_BACK_CR;
							}
							else
							{
								autoMove = LS_A_BACK;
							}
						}
					}
				}
			}
		}
	}
	return autoMove;
}

qboolean PM_InSecondaryStyle( void )
{
	if ( pm->ps->saber[0].numBlades > 1
		&& pm->ps->saber[0].singleBladeStyle
		&& (pm->ps->saber[0].stylesForbidden&(1<<pm->ps->saber[0].singleBladeStyle))
		&& pm->ps->saberAnimLevel == pm->ps->saber[0].singleBladeStyle )
	{
		return qtrue;
	}

	if ( pm->ps->dualSabers
		&& !pm->ps->saber[1].Active() )//pm->ps->saberAnimLevel != SS_DUAL )
	{
		return qtrue;
	}
	return qfalse;
}

saberMoveName_t PM_SaberLungeAttackMove( qboolean fallbackToNormalLunge )
{
	G_DrainPowerForSpecialMove( pm->gent, FP_SABER_OFFENSE, SABER_ALT_ATTACK_POWER_FB );

	//see if we have an overridden (or cancelled) lunge move
	if ( pm->ps->saber[0].lungeAtkMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].lungeAtkMove != LS_NONE )
		{
			return (saberMoveName_t)pm->ps->saber[0].lungeAtkMove;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].lungeAtkMove != LS_INVALID )
		{
			if ( pm->ps->saber[1].lungeAtkMove != LS_NONE )
			{
				return (saberMoveName_t)pm->ps->saber[1].lungeAtkMove;
			}
		}
	}
	//no overrides, cancelled?
	if ( pm->ps->saber[0].lungeAtkMove == LS_NONE )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].lungeAtkMove == LS_NONE )
		{
			return LS_NONE;
		}
	}
	//do normal checks
	if ( pm->gent->client->NPC_class == CLASS_ALORA && !Q_irand( 0, 3 ) )
	{//alora NPC
		return LS_SPINATTACK_ALORA;
	}
	else
	{
		if ( pm->ps->dualSabers )
		{
			return LS_SPINATTACK_DUAL;
		}
		switch ( pm->ps->saberAnimLevel )
		{
		case SS_DUAL:
			return LS_SPINATTACK_DUAL;
			break;
		case SS_STAFF:
			return LS_SPINATTACK;
			break;
		default://normal lunge
			if ( fallbackToNormalLunge )
			{
				vec3_t fwdAngles, jumpFwd;

				VectorCopy( pm->ps->viewangles, fwdAngles );
				fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
				//do the lunge
				AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
				VectorScale( jumpFwd, 150, pm->ps->velocity );
				pm->ps->velocity[2] = 50;
				PM_AddEvent( EV_JUMP );

				return LS_A_LUNGE;
			}
			break;
		}
	}
	return LS_NONE;
}

qboolean PM_CheckLungeAttackMove( void )
{
	//check to see if it's cancelled?
	if ( pm->ps->saber[0].lungeAtkMove == LS_NONE )
	{
		if ( pm->ps->dualSabers )
		{
			if ( pm->ps->saber[1].lungeAtkMove == LS_NONE
				|| pm->ps->saber[1].lungeAtkMove == LS_INVALID )
			{
				return qfalse;
			}
		}
		else
		{
			return qfalse;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].lungeAtkMove == LS_NONE )
		{
			if ( pm->ps->saber[0].lungeAtkMove == LS_NONE
				|| pm->ps->saber[0].lungeAtkMove == LS_INVALID )
			{
				return qfalse;
			}
		}
	}
	//do normal checks
	if ( pm->ps->saberAnimLevel == SS_FAST//fast
		|| pm->ps->saberAnimLevel == SS_DUAL//dual
		|| pm->ps->saberAnimLevel == SS_STAFF //staff
		|| pm->ps->saberAnimLevel == SS_DESANN
		|| pm->ps->dualSabers )
	{//alt+back+attack using fast, dual or staff attacks
		if ( pm->ps->clientNum >= MAX_CLIENTS && !PM_ControlledByPlayer() )
		{//NPC
			if ( pm->cmd.upmove < 0 || (pm->ps->pm_flags&PMF_DUCKED) )
			{//ducking
				if ( pm->ps->legsAnim == BOTH_STAND2
					|| pm->ps->legsAnim == BOTH_SABERFAST_STANCE
					|| pm->ps->legsAnim == BOTH_SABERSLOW_STANCE
					|| pm->ps->legsAnim == BOTH_SABERSTAFF_STANCE
					|| pm->ps->legsAnim == BOTH_SABERDUAL_STANCE
					|| (level.time-pm->ps->lastStationary) <= 500  )
				{//standing or just stopped standing
					if ( pm->gent
						&& pm->gent->NPC //NPC
						&& pm->gent->NPC->rank >= RANK_LT_JG //high rank
						&& ( pm->gent->NPC->rank == RANK_LT_JG || Q_irand( -3, pm->gent->NPC->rank ) >= RANK_LT_JG )//Q_irand( 0, pm->gent->NPC->rank ) >= RANK_LT_JG )
						&& !Q_irand( 0, 3-g_spskill->integer ) )
					{//only fencer and higher can do this
						if ( pm->ps->saberAnimLevel == SS_DESANN )
						{
							if ( !Q_irand( 0, 4 ) )
							{
								return qtrue;
							}
						}
						else
						{
							return qtrue;
						}
					}
				}
			}
		}
		else
		{//player
			if ( G_TryingLungeAttack( pm->gent, &pm->cmd )
				&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_FB )/*pm->ps->forcePower >= SABER_ALT_ATTACK_POWER_FB*/ )//have enough force power to pull it off
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

saberMoveName_t PM_SaberJumpForwardAttackMove( void )
{
	G_DrainPowerForSpecialMove( pm->gent, FP_LEVITATION, SABER_ALT_ATTACK_POWER_FB );

	//see if we have an overridden (or cancelled) kata move
	if ( pm->ps->saber[0].jumpAtkFwdMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)pm->ps->saber[0].jumpAtkFwdMove;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkFwdMove != LS_INVALID )
		{
			if ( pm->ps->saber[1].jumpAtkFwdMove != LS_NONE )
			{
				return (saberMoveName_t)pm->ps->saber[1].jumpAtkFwdMove;
			}
		}
	}
	//no overrides, cancelled?
	if ( pm->ps->saber[0].jumpAtkFwdMove == LS_NONE )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkFwdMove == LS_NONE )
		{
			return LS_NONE;
		}
	}
	if ( pm->ps->saberAnimLevel == SS_DUAL
		|| pm->ps->saberAnimLevel == SS_STAFF )
	{
		pm->cmd.upmove = 0;//no jump just yet

		if ( pm->ps->saberAnimLevel == SS_STAFF )
		{
			if ( Q_irand(0, 1) )
			{
				return LS_JUMPATTACK_STAFF_LEFT;
			}
			else
			{
				return LS_JUMPATTACK_STAFF_RIGHT;
			}
		}

		return LS_JUMPATTACK_DUAL;
	}
	else
	{
		vec3_t fwdAngles, jumpFwd;

		VectorCopy( pm->ps->viewangles, fwdAngles );
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
		VectorScale( jumpFwd, 200, pm->ps->velocity );
		pm->ps->velocity[2] = 180;
		pm->ps->forceJumpZStart = pm->ps->origin[2];//so we don't take damage if we land at same height
		pm->ps->pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;

		//FIXME: NPCs yell?
		PM_AddEvent( EV_JUMP );
		G_SoundOnEnt( pm->gent, CHAN_BODY, "sound/weapons/force/jump.wav" );
		pm->cmd.upmove = 0;

		return LS_A_JUMP_T__B_;
	}
}

qboolean PM_CheckJumpForwardAttackMove( void )
{
	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		return qfalse;
	}

	//check to see if it's cancelled?
	if ( pm->ps->saber[0].jumpAtkFwdMove == LS_NONE )
	{
		if ( pm->ps->dualSabers )
		{
			if ( pm->ps->saber[1].jumpAtkFwdMove == LS_NONE
				|| pm->ps->saber[1].jumpAtkFwdMove == LS_INVALID )
			{
				return qfalse;
			}
		}
		else
		{
			return qfalse;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkFwdMove == LS_NONE )
		{
			if ( pm->ps->saber[0].jumpAtkFwdMove == LS_NONE
				|| pm->ps->saber[0].jumpAtkFwdMove == LS_INVALID )
			{
				return qfalse;
			}
		}
	}
	//do normal checks

	if ( pm->cmd.forwardmove > 0 //going forward
		&& pm->ps->forceRageRecoveryTime < pm->cmd.serverTime	//not in a force Rage recovery period
		&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //can force jump
		&& pm->gent && !(pm->gent->flags&FL_LOCK_PLAYER_WEAPONS) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
		&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=250) //on ground or just jumped (if not player)
		)
	{
		if ( pm->ps->saberAnimLevel == SS_DUAL
			|| pm->ps->saberAnimLevel == SS_STAFF )
		{//dual and staff
			if ( !PM_SaberInTransitionAny( pm->ps->saberMove ) //not going to/from/between an attack anim
				&& !PM_SaberInAttack( pm->ps->saberMove ) //not in attack anim
				&& pm->ps->weaponTime <= 0//not busy
				&& (pm->cmd.buttons&BUTTON_ATTACK) )//want to attack
			{
				if ( pm->ps->clientNum >= MAX_CLIENTS && !PM_ControlledByPlayer() )
				{//NPC
					if ( pm->cmd.upmove > 0 || (pm->ps->pm_flags&PMF_JUMPING) )//jumping NPC
					{
						if ( pm->gent
							&& pm->gent->NPC
							&& (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT) )
						{
							return qtrue;
						}
					}
				}
				else
				{//PLAYER
					if ( G_TryingJumpForwardAttack( pm->gent, &pm->cmd )
						&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_FB ) )//have enough power to attack
					{
						return qtrue;
					}
				}
			}
		}
		//check strong
		else if ( pm->ps->saberAnimLevel == SS_STRONG //strong style
			|| pm->ps->saberAnimLevel == SS_DESANN )//desann
		{
			if ( //&& !PM_InKnockDown( pm->ps )
				!pm->ps->dualSabers
				//&& (pm->ps->legsAnim == BOTH_STAND2||pm->ps->legsAnim == BOTH_SABERFAST_STANCE||pm->ps->legsAnim == BOTH_SABERSLOW_STANCE||level.time-pm->ps->lastStationary<=500)//standing or just started moving
				)
			{//strong attack: jump-hack
				/*
				if ( pm->ps->legsAnim == BOTH_STAND2
					|| pm->ps->legsAnim == BOTH_SABERFAST_STANCE
					|| pm->ps->legsAnim == BOTH_SABERSLOW_STANCE
					|| level.time-pm->ps->lastStationary <= 250 )//standing or just started moving
				*/
				if ( pm->ps->clientNum >= MAX_CLIENTS && !PM_ControlledByPlayer() )
				{//NPC
					if ( pm->cmd.upmove > 0 || (pm->ps->pm_flags&PMF_JUMPING) )//NPC jumping
					{
						if ( pm->gent
							&& pm->gent->NPC
							&& (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT) )
						{//only acrobat or boss and higher can do this
							if ( pm->ps->legsAnim == BOTH_STAND2
								|| pm->ps->legsAnim == BOTH_SABERFAST_STANCE
								|| pm->ps->legsAnim == BOTH_SABERSLOW_STANCE
								|| level.time-pm->ps->lastStationary <= 250 )
							{//standing or just started moving
								if ( pm->gent->client
									&& pm->gent->client->NPC_class == CLASS_DESANN )
								{
									if ( !Q_irand( 0, 1 ) )
									{
										return qtrue;
									}
								}
								else
								{
									return qtrue;
								}
							}
						}
					}
				}
				else
				{//player
					if ( G_TryingJumpForwardAttack( pm->gent, &pm->cmd )
						&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_FB ) )
					{
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

saberMoveName_t PM_SaberFlipOverAttackMove( void )
{
	//see if we have an overridden (or cancelled) kata move
	if ( pm->ps->saber[0].jumpAtkFwdMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)pm->ps->saber[0].jumpAtkFwdMove;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkFwdMove != LS_INVALID )
		{
			if ( pm->ps->saber[1].jumpAtkFwdMove != LS_NONE )
			{
				return (saberMoveName_t)pm->ps->saber[1].jumpAtkFwdMove;
			}
		}
	}
	//no overrides, cancelled?
	if ( pm->ps->saber[0].jumpAtkFwdMove == LS_NONE )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkFwdMove == LS_NONE )
		{
			return LS_NONE;
		}
	}
	//FIXME: check above for room enough to jump!
	//FIXME: while in this jump, keep velocity[2] at a minimum until the end of the anim
	vec3_t fwdAngles, jumpFwd;

	VectorCopy( pm->ps->viewangles, fwdAngles );
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
	VectorScale( jumpFwd, 150, pm->ps->velocity );
	pm->ps->velocity[2] = 250;
	//250 is normalized for a standing enemy at your z level, about 64 tall... adjust for actual maxs[2]-mins[2] of enemy and for zdiff in origins
	if ( pm->gent && pm->gent->enemy )
	{	//go higher for taller enemies
		pm->ps->velocity[2] *= (pm->gent->enemy->maxs[2]-pm->gent->enemy->mins[2])/64.0f;
		//go higher for enemies higher than you, lower for those lower than you
		float zDiff = pm->gent->enemy->currentOrigin[2] - pm->ps->origin[2];
		pm->ps->velocity[2] += (zDiff)*1.5f;
		//clamp to decent-looking values
		//FIXME: still jump too low sometimes
		if ( zDiff <= 0 && pm->ps->velocity[2] < 200 )
		{//if we're on same level, don't let me jump so low, I clip into the ground
			pm->ps->velocity[2] = 200;
		}
		else if ( pm->ps->velocity[2] < 50 )
		{
			pm->ps->velocity[2] = 50;
		}
		else if ( pm->ps->velocity[2] > 400 )
		{
			pm->ps->velocity[2] = 400;
		}
	}
	pm->ps->forceJumpZStart = pm->ps->origin[2];//so we don't take damage if we land at same height
	pm->ps->pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;

	//FIXME: NPCs yell?
	PM_AddEvent( EV_JUMP );
	G_SoundOnEnt( pm->gent, CHAN_BODY, "sound/weapons/force/jump.wav" );
	pm->cmd.upmove = 0;
	//FIXME: don't allow this to land on other people

	pm->gent->angle = pm->ps->viewangles[YAW];//so we know what yaw we started this at

	G_DrainPowerForSpecialMove( pm->gent, FP_LEVITATION, SABER_ALT_ATTACK_POWER_FB );

	if ( Q_irand( 0, 1 ) )
	{
		return LS_A_FLIP_STAB;
	}
	else
	{
		return LS_A_FLIP_SLASH;
	}
}

qboolean PM_CheckFlipOverAttackMove( qboolean checkEnemy )
{
	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		return qfalse;
	}
	//check to see if it's cancelled?
	if ( pm->ps->saber[0].jumpAtkFwdMove == LS_NONE )
	{
		if ( pm->ps->dualSabers )
		{
			if ( pm->ps->saber[1].jumpAtkFwdMove == LS_NONE
				|| pm->ps->saber[1].jumpAtkFwdMove == LS_INVALID )
			{
				return qfalse;
			}
		}
		else
		{
			return qfalse;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkFwdMove == LS_NONE )
		{
			if ( pm->ps->saber[0].jumpAtkFwdMove == LS_NONE
				|| pm->ps->saber[0].jumpAtkFwdMove == LS_INVALID )
			{
				return qfalse;
			}
		}
	}
	//do normal checks

	if ( (pm->ps->saberAnimLevel == SS_MEDIUM //medium
		|| pm->ps->saberAnimLevel == SS_TAVION )//tavion
		&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //can force jump
		&& !(pm->gent->flags&FL_LOCK_PLAYER_WEAPONS) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
		&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=250) //on ground or just jumped
		)
	{
		qboolean tryMove = qfalse;
		if ( pm->ps->clientNum >= MAX_CLIENTS && !PM_ControlledByPlayer() )
		{//NPC
			if ( pm->cmd.upmove > 0//want to jump
				|| (pm->ps->pm_flags&PMF_JUMPING) )//jumping
			{//flip over-forward down-attack
				if ( (pm->gent->NPC
					&& (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT)
					&& !Q_irand(0, 2) ) )//NPC who can do this, 33% chance
				{//only player or acrobat or boss and higher can do this
					tryMove = qtrue;
				}
			}
		}
		else
		{//player
			if ( G_TryingJumpForwardAttack( pm->gent, &pm->cmd )
				&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_FB ) )//have enough power
			{
				if ( !pm->cmd.rightmove )
				{
					if ( pm->ps->legsAnim == BOTH_JUMP1
						|| pm->ps->legsAnim == BOTH_FORCEJUMP1
						|| pm->ps->legsAnim == BOTH_INAIR1
						|| pm->ps->legsAnim == BOTH_FORCEINAIR1 )
					{//in a non-flip forward jump
						tryMove = qtrue;
					}
				}
			}
		}

		if ( tryMove )
		{
			if ( !checkEnemy )
			{//based just on command input
				return qtrue;
			}
			else
			{//based on presence of enemy
				if ( pm->gent->enemy )//have an enemy
				{
					vec3_t fwdAngles = {0,pm->ps->viewangles[YAW],0};
					if ( pm->gent->enemy->health > 0
						&& pm->ps->forceRageRecoveryTime < pm->cmd.serverTime	//not in a force Rage recovery period
						&& pm->gent->enemy->maxs[2] > 12
						&& (!pm->gent->enemy->client || !PM_InKnockDownOnGround( &pm->gent->enemy->client->ps ) )
						&& DistanceSquared( pm->gent->currentOrigin, pm->gent->enemy->currentOrigin ) < 10000
						&& InFront( pm->gent->enemy->currentOrigin, pm->gent->currentOrigin, fwdAngles, 0.3f ) )
					{//enemy must be alive, not low to ground, close and in front
						return qtrue;
					}
				}
				return qfalse;
			}
		}
	}
	return qfalse;
}

saberMoveName_t PM_SaberBackflipAttackMove( void )
{
	//see if we have an overridden (or cancelled) kata move
	if ( pm->ps->saber[0].jumpAtkBackMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].jumpAtkBackMove != LS_NONE )
		{
			return (saberMoveName_t)pm->ps->saber[0].jumpAtkBackMove;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkBackMove != LS_INVALID )
		{
			if ( pm->ps->saber[1].jumpAtkBackMove != LS_NONE )
			{
				return (saberMoveName_t)pm->ps->saber[1].jumpAtkBackMove;
			}
		}
	}
	//no overrides, cancelled?
	if ( pm->ps->saber[0].jumpAtkBackMove == LS_NONE )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkBackMove == LS_NONE )
		{
			return LS_NONE;
		}
	}
	pm->cmd.upmove = 0;//no jump just yet
	return LS_A_BACKFLIP_ATK;
}

qboolean PM_CheckBackflipAttackMove( void )
{
	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		return qfalse;
	}

	//check to see if it's cancelled?
	if ( pm->ps->saber[0].jumpAtkBackMove == LS_NONE )
	{
		if ( pm->ps->dualSabers )
		{
			if ( pm->ps->saber[1].jumpAtkBackMove == LS_NONE
				|| pm->ps->saber[1].jumpAtkBackMove == LS_INVALID )
			{
				return qfalse;
			}
		}
		else
		{
			return qfalse;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].jumpAtkBackMove == LS_NONE )
		{
			if ( pm->ps->saber[0].jumpAtkBackMove == LS_NONE
				|| pm->ps->saber[0].jumpAtkBackMove == LS_INVALID )
			{
				return qfalse;
			}
		}
	}
	//do normal checks

	if ( pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //can force jump
		&& pm->ps->forceRageRecoveryTime < pm->cmd.serverTime	//not in a force Rage recovery period
		&& pm->gent && !(pm->gent->flags&FL_LOCK_PLAYER_WEAPONS) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
		//&& (pm->ps->legsAnim == BOTH_SABERSTAFF_STANCE || level.time-pm->ps->lastStationary<=250)//standing or just started moving
		&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=250) )//on ground or just jumped (if not player)
	{
		if ( pm->cmd.forwardmove < 0 //moving backwards
			&& pm->ps->saberAnimLevel == SS_STAFF //using staff
			&& (pm->cmd.upmove > 0 || (pm->ps->pm_flags&PMF_JUMPING)) )//jumping
		{//jumping backwards and using staff
			if ( !PM_SaberInTransitionAny( pm->ps->saberMove ) //not going to/from/between an attack anim
				&& !PM_SaberInAttack( pm->ps->saberMove ) //not in attack anim
				&& pm->ps->weaponTime <= 0//not busy
				&& (pm->cmd.buttons&BUTTON_ATTACK) )//want to attack
			{//not already attacking
				if ( pm->ps->clientNum >= MAX_CLIENTS && !PM_ControlledByPlayer() )
				{//NPC
					if ( pm->gent
						&& pm->gent->NPC
						&& (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT) )
					{//acrobat or boss and higher can do this
						return qtrue;
					}
				}
				else
				{//player
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

saberMoveName_t PM_CheckDualSpinProtect( void )
{
	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		return LS_NONE;
	}

	//see if we have an overridden (or cancelled) kata move
	if ( pm->ps->saber[0].kataMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].kataMove != LS_NONE )
		{
			return (saberMoveName_t)pm->ps->saber[0].kataMove;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].kataMove != LS_INVALID )
		{
			if ( pm->ps->saber[1].kataMove != LS_NONE )
			{
				return (saberMoveName_t)pm->ps->saber[1].kataMove;
			}
		}
	}
	//no overrides, cancelled?
	if ( pm->ps->saber[0].kataMove == LS_NONE )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].kataMove == LS_NONE )
		{
			return LS_NONE;
		}
	}
	//do normal checks
	if ( pm->ps->saberMove == LS_READY//ready
		//&& (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer())//PLAYER ONLY...?
		//&& pm->ps->viewangles[0] > 30 //looking down
		&& pm->ps->saberAnimLevel == SS_DUAL//using dual saber style
		&& pm->ps->saber[0].Active() && pm->ps->saber[1].Active()//both sabers on
		//&& pm->ps->forcePowerLevel[FP_PUSH]>=FORCE_LEVEL_3//force push 3
		//&& ((pm->ps->forcePowersActive&(1<<FP_PUSH))||pm->ps->forcePowerDebounce[FP_PUSH]>level.time)//force-pushing
		&& G_TryingKataAttack( pm->gent, &pm->cmd )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS)//holding focus
		&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER, qtrue )//pm->ps->forcePower >= SABER_ALT_ATTACK_POWER//DUAL_SPIN_PROTECT_POWER//force push 3
		&& (pm->cmd.buttons&BUTTON_ATTACK)//pressing attack
		)
	{//FIXME: some NPC logic to do this?
		/*
		if ( (pm->ps->pm_flags&PMF_DUCKED||pm->cmd.upmove<0)//crouching
			&& g_crosshairEntNum >= ENTITYNUM_WORLD )
		*/
		{
			if ( pm->gent )
			{
				G_DrainPowerForSpecialMove( pm->gent, FP_PUSH, SABER_ALT_ATTACK_POWER, qtrue );//drain the required force power
			}
			return LS_DUAL_SPIN_PROTECT;
		}
	}
	return LS_NONE;
}

saberMoveName_t PM_CheckStaffKata( void )
{
	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		return LS_NONE;
	}

	//see if we have an overridden (or cancelled) kata move
	if ( pm->ps->saber[0].kataMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].kataMove != LS_NONE )
		{
			return (saberMoveName_t)pm->ps->saber[0].kataMove;
		}
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].kataMove != LS_INVALID )
		{
			if ( pm->ps->saber[1].kataMove != LS_NONE )
			{
				return (saberMoveName_t)pm->ps->saber[1].kataMove;
			}
		}
	}
	//no overrides, cancelled?
	if ( pm->ps->saber[0].kataMove == LS_NONE )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers )
	{
		if ( pm->ps->saber[1].kataMove == LS_NONE )
		{
			return LS_NONE;
		}
	}
	//do normal checks
	if ( pm->ps->saberMove == LS_READY//ready
		//&& (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer())//PLAYER ONLY...?
		//&& pm->ps->viewangles[0] > 30 //looking down
		&& pm->ps->saberAnimLevel == SS_STAFF//using dual saber style
		&& pm->ps->saber[0].Active()//saber on
		//&& pm->ps->forcePowerLevel[FP_PUSH]>=FORCE_LEVEL_3//force push 3
		//&& ((pm->ps->forcePowersActive&(1<<FP_PUSH))||pm->ps->forcePowerDebounce[FP_PUSH]>level.time)//force-pushing
		&& G_TryingKataAttack( pm->gent, &pm->cmd )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS)//holding focus
		&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER, qtrue )//pm->ps->forcePower >= SABER_ALT_ATTACK_POWER//DUAL_SPIN_PROTECT_POWER//force push 3
		&& (pm->cmd.buttons&BUTTON_ATTACK)//pressing attack
		)
	{//FIXME: some NPC logic to do this?
		/*
		if ( (pm->ps->pm_flags&PMF_DUCKED||pm->cmd.upmove<0)//crouching
			&& g_crosshairEntNum >= ENTITYNUM_WORLD )
		*/
		{
			if ( pm->gent )
			{
				G_DrainPowerForSpecialMove( pm->gent, FP_LEVITATION, SABER_ALT_ATTACK_POWER, qtrue );//drain the required force power
			}
			return LS_STAFF_SOULCAL;
		}
	}
	return LS_NONE;
}

extern qboolean WP_ForceThrowable( gentity_t *ent, gentity_t *forwardEnt, gentity_t *self, qboolean pull, float cone, float radius, vec3_t forward );
saberMoveName_t PM_CheckPullAttack( void )
{
	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		return LS_NONE;
	}

	if ( (pm->ps->saber[0].saberFlags&SFL_NO_PULL_ATTACK) )
	{
		return LS_NONE;
	}
	if ( pm->ps->dualSabers
		&& (pm->ps->saber[1].saberFlags&SFL_NO_PULL_ATTACK) )
	{
		return LS_NONE;
	}

	if ( (pm->ps->saberMove == LS_READY||PM_SaberInReturn(pm->ps->saberMove)||PM_SaberInReflect(pm->ps->saberMove))//ready
		//&& (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer())//PLAYER ONLY
		&& pm->ps->saberAnimLevel >= SS_FAST//single saber styles - FIXME: Tavion?
		&& pm->ps->saberAnimLevel <= SS_STRONG//single saber styles - FIXME: Tavion?
		&& G_TryingPullAttack( pm->gent, &pm->cmd, qfalse )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS)//holding focus
		//&& pm->cmd.forwardmove<0//pulling back
		&& (pm->cmd.buttons&BUTTON_ATTACK)//attacking
		&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_FB )//pm->ps->forcePower >= SABER_ALT_ATTACK_POWER_FB//have enough power
		)
	{//FIXME: some NPC logic to do this?
		qboolean doMove = g_saberNewControlScheme->integer?qtrue:qfalse;//in new control scheme, can always do this, even if there's no-one to do it to
		if ( g_saberNewControlScheme->integer
			|| g_crosshairEntNum < ENTITYNUM_WORLD )//in old control scheme, there has to be someone there
		{
			saberMoveName_t pullAttackMove = LS_NONE;
			if ( pm->ps->saberAnimLevel == SS_FAST )
			{
				pullAttackMove = LS_PULL_ATTACK_STAB;
			}
			else
			{
				pullAttackMove = LS_PULL_ATTACK_SWING;
			}

			if ( g_crosshairEntNum < ENTITYNUM_WORLD
				&& pm->gent && pm->gent->client )
			{
				gentity_t *targEnt = &g_entities[g_crosshairEntNum];
				if ( targEnt->client
					&& targEnt->health > 0
					//FIXME: check other things like in knockdown, saberlock, uninterruptable anims, etc.
					&& !PM_InOnGroundAnim( &targEnt->client->ps )
					&& !PM_LockedAnim( targEnt->client->ps.legsAnim )
					&& !PM_SuperBreakLoseAnim( targEnt->client->ps.legsAnim )
					&& !PM_SuperBreakWinAnim( targEnt->client->ps.legsAnim )
					&& targEnt->client->ps.saberLockTime <= 0
					&& WP_ForceThrowable( targEnt, targEnt, pm->gent, qtrue, 1.0f, 0.0f, NULL ) )
				{
					if ( !g_saberNewControlScheme->integer )
					{//in old control scheme, make sure they're close or far enough away for the move we'll be doing
						float targDist = Distance( targEnt->currentOrigin, pm->ps->origin );
						if ( pullAttackMove == LS_PULL_ATTACK_STAB )
						{//must be closer than 512
							if ( targDist > 384.0f )
							{
								return LS_NONE;
							}
						}
						else//if ( pullAttackMove == LS_PULL_ATTACK_SWING )
						{//must be farther than 256
							if ( targDist > 512.0f )
							{
								return LS_NONE;
							}
							if ( targDist < 192.0f )
							{
								return LS_NONE;
							}
						}
					}

					vec3_t targAngles = {0,targEnt->client->ps.viewangles[YAW],0};
					if ( InFront( pm->ps->origin, targEnt->currentOrigin, targAngles ) )
					{
						NPC_SetAnim( targEnt, SETANIM_BOTH, BOTH_PULLED_INAIR_F, SETANIM_FLAG_OVERRIDE, SETANIM_FLAG_HOLD );
					}
					else
					{
						NPC_SetAnim( targEnt, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE, SETANIM_FLAG_HOLD );
					}
					//hold the anim until I'm with done pull anim
					targEnt->client->ps.legsAnimTimer = targEnt->client->ps.torsoAnimTimer = PM_AnimLength( pm->gent->client->clientInfo.animFileIndex, (animNumber_t)saberMoveData[pullAttackMove].animToUse );
					//set pullAttackTime
					pm->gent->client->ps.pullAttackTime = targEnt->client->ps.pullAttackTime = level.time+targEnt->client->ps.legsAnimTimer;
					//make us know about each other
					pm->gent->client->ps.pullAttackEntNum = g_crosshairEntNum;
					targEnt->client->ps.pullAttackEntNum = pm->ps->clientNum;
					//do effect and sound on me
					pm->ps->powerups[PW_FORCE_PUSH] = level.time + 1000;
					if ( pm->gent )
					{
						G_Sound( pm->gent, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
					}
					doMove = qtrue;
				}
			}
			if ( doMove )
			{
				if ( pm->gent )
				{
					G_DrainPowerForSpecialMove( pm->gent, FP_PULL, SABER_ALT_ATTACK_POWER_FB );
				}
				return pullAttackMove;
			}
		}
	}
	return LS_NONE;
}

saberMoveName_t PM_CheckPlayerAttackFromParry( int curmove )
{
	if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() )
	{
		if ( curmove >= LS_PARRY_UP
			&& curmove <= LS_REFLECT_LL )
		{//in a parry
			switch ( saberMoveData[curmove].endQuad )
			{
			case Q_T:
				return LS_A_T2B;
				break;
			case Q_TR:
				return LS_A_TR2BL;
				break;
			case Q_TL:
				return LS_A_TL2BR;
				break;
			case Q_BR:
				return LS_A_BR2TL;
				break;
			case Q_BL:
				return LS_A_BL2TR;
				break;
			//shouldn't be a parry that ends at L, R or B
			}
		}
	}
	return LS_NONE;
}


saberMoveName_t PM_SaberAttackForMovement( int forwardmove, int rightmove, int curmove )
{
	qboolean noSpecials = qfalse;

	if ( pm->ps->clientNum < MAX_CLIENTS
		&& PM_InSecondaryStyle() )
	{
		noSpecials = qtrue;
	}

	saberMoveName_t overrideJumpRightAttackMove = LS_INVALID;
	if ( pm->ps->saber[0].jumpAtkRightMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].jumpAtkRightMove != LS_NONE )
		{//actually overriding
			overrideJumpRightAttackMove = (saberMoveName_t)pm->ps->saber[0].jumpAtkRightMove;
		}
		else if ( pm->ps->dualSabers
			&& pm->ps->saber[1].jumpAtkRightMove > LS_NONE )
		{//would be cancelling it, but check the second saber, too
			overrideJumpRightAttackMove = (saberMoveName_t)pm->ps->saber[1].jumpAtkRightMove;
		}
		else
		{//nope, just cancel it
			overrideJumpRightAttackMove = LS_NONE;
		}
	}
	else if ( pm->ps->dualSabers
		&& pm->ps->saber[1].jumpAtkRightMove != LS_INVALID )
	{//first saber not overridden, check second
		overrideJumpRightAttackMove = (saberMoveName_t)pm->ps->saber[1].jumpAtkRightMove;
	}

	saberMoveName_t overrideJumpLeftAttackMove = LS_INVALID;
	if ( pm->ps->saber[0].jumpAtkLeftMove != LS_INVALID )
	{
		if ( pm->ps->saber[0].jumpAtkLeftMove != LS_NONE )
		{//actually overriding
			overrideJumpLeftAttackMove = (saberMoveName_t)pm->ps->saber[0].jumpAtkLeftMove;
		}
		else if ( pm->ps->dualSabers
			&& pm->ps->saber[1].jumpAtkLeftMove > LS_NONE )
		{//would be cancelling it, but check the second saber, too
			overrideJumpLeftAttackMove = (saberMoveName_t)pm->ps->saber[1].jumpAtkLeftMove;
		}
		else
		{//nope, just cancel it
			overrideJumpLeftAttackMove = LS_NONE;
		}
	}
	else if ( pm->ps->dualSabers
		&& pm->ps->saber[1].jumpAtkLeftMove != LS_INVALID )
	{//first saber not overridden, check second
		overrideJumpLeftAttackMove = (saberMoveName_t)pm->ps->saber[1].jumpAtkLeftMove;
	}
	if ( rightmove > 0 )
	{//moving right
		if ( !noSpecials
			&& overrideJumpRightAttackMove != LS_NONE
			&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=250) //on ground or just jumped
			&& (pm->cmd.buttons&BUTTON_ATTACK)//hitting attack
			&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0//have force jump 1 at least
			&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_LR )//pm->ps->forcePower >= SABER_ALT_ATTACK_POWER_LR//have enough power
			&& (((pm->ps->clientNum>=MAX_CLIENTS&&!PM_ControlledByPlayer())&&pm->cmd.upmove > 0)//jumping NPC
				||((pm->ps->clientNum<MAX_CLIENTS||PM_ControlledByPlayer())&&G_TryingCartwheel(pm->gent, &pm->cmd)/*(pm->cmd.buttons&BUTTON_FORCE_FOCUS)*/)) )//focus-holding player
		{//cartwheel right
			vec3_t right, fwdAngles = {0, pm->ps->viewangles[YAW], 0};
			if ( pm->gent )
			{
				G_DrainPowerForSpecialMove( pm->gent, FP_LEVITATION, SABER_ALT_ATTACK_POWER_LR );
			}
			pm->cmd.upmove = 0;
			if ( overrideJumpRightAttackMove != LS_INVALID )
			{//overridden with another move
				return overrideJumpRightAttackMove;
			}
			else if ( pm->ps->saberAnimLevel == SS_STAFF )
			{
				AngleVectors( fwdAngles, NULL, right, NULL );
				pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
				VectorMA( pm->ps->velocity, 190, right, pm->ps->velocity );
				return LS_BUTTERFLY_RIGHT;
			}
			else
			{
				if ( !(pm->ps->saber[0].saberFlags&SFL_NO_CARTWHEELS)
					&& (!pm->ps->dualSabers || !(pm->ps->saber[1].saberFlags&SFL_NO_CARTWHEELS)) )
				{//okay to do cartwheels with this saber
					/*
					if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
					{//still on ground
						VectorClear( pm->ps->velocity );
						return LS_JUMPATTACK_CART_RIGHT;
					}
					else
					*/
					{//in air
						AngleVectors( fwdAngles, NULL, right, NULL );
						pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
						VectorMA( pm->ps->velocity, 190, right, pm->ps->velocity );
						PM_SetJumped( JUMP_VELOCITY, qtrue );
						return LS_JUMPATTACK_ARIAL_RIGHT;
					}
				}
			}
		}
		else if ( pm->ps->legsAnim != BOTH_CARTWHEEL_RIGHT
			&& pm->ps->legsAnim != BOTH_ARIAL_RIGHT )
		{//not in a cartwheel/arial
			if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
			{//player
				if ( G_TryingSpecial(pm->gent, &pm->cmd)/*(pm->cmd.buttons&BUTTON_FORCE_FOCUS)*/ )//Holding focus
				{//if no special worked, do nothing
					return LS_NONE;
				}
			}
			//checked all special attacks, if we're in a parry, attack from that move
			saberMoveName_t parryAttackMove = PM_CheckPlayerAttackFromParry( curmove );
			if ( parryAttackMove != LS_NONE )
			{
				return parryAttackMove;
			}
			//check regular attacks
			if ( forwardmove > 0 )
			{//forward right = TL2BR slash
				return LS_A_TL2BR;
			}
			else if ( forwardmove < 0 )
			{//backward right = BL2TR uppercut
				return LS_A_BL2TR;
			}
			else
			{//just right is a left slice
				return LS_A_L2R;
			}
		}
	}
	else if ( rightmove < 0 )
	{//moving left
		if ( !noSpecials
			&& overrideJumpLeftAttackMove != LS_NONE
			&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=250) //on ground or just jumped
			&& (pm->cmd.buttons&BUTTON_ATTACK)//hitting attack
			&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0//have force jump 1 at least
			&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_LR )//pm->ps->forcePower >= SABER_ALT_ATTACK_POWER_LR//have enough power
			&& (((pm->ps->clientNum>=MAX_CLIENTS&&!PM_ControlledByPlayer())&&pm->cmd.upmove > 0)//jumping NPC
				||((pm->ps->clientNum<MAX_CLIENTS||PM_ControlledByPlayer())&&G_TryingCartwheel(pm->gent, &pm->cmd)/*(pm->cmd.buttons&BUTTON_FORCE_FOCUS)*/)) )//focus-holding player
		{//cartwheel left
			vec3_t right, fwdAngles = {0, pm->ps->viewangles[YAW], 0};
			if ( pm->gent )
			{
				G_DrainPowerForSpecialMove( pm->gent, FP_LEVITATION, SABER_ALT_ATTACK_POWER_LR );
			}
			pm->cmd.upmove = 0;
			if ( overrideJumpRightAttackMove != LS_INVALID )
			{//overridden with another move
				return overrideJumpRightAttackMove;
			}
			else if ( pm->ps->saberAnimLevel == SS_STAFF )
			{
				AngleVectors( fwdAngles, NULL, right, NULL );
				pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
				VectorMA( pm->ps->velocity, -190, right, pm->ps->velocity );
				return LS_BUTTERFLY_LEFT;
			}
			else
			{
				if ( !(pm->ps->saber[0].saberFlags&SFL_NO_CARTWHEELS)
					&& (!pm->ps->dualSabers || !(pm->ps->saber[1].saberFlags&SFL_NO_CARTWHEELS)) )
				{//okay to do cartwheels with this saber
					/*
					if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
					{//still on ground
						VectorClear( pm->ps->velocity );
						return LS_JUMPATTACK_ARIAL_LEFT;
					}
					else
					*/
					{
						AngleVectors( fwdAngles, NULL, right, NULL );
						pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
						VectorMA( pm->ps->velocity, -190, right, pm->ps->velocity );
						PM_SetJumped( JUMP_VELOCITY, qtrue );
						return LS_JUMPATTACK_CART_LEFT;
					}
				}
			}
		}
		else if ( pm->ps->legsAnim != BOTH_CARTWHEEL_LEFT
			&& pm->ps->legsAnim != BOTH_ARIAL_LEFT )
		{//not in a left cartwheel/arial
			if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
			{//player
				if ( G_TryingSpecial(pm->gent, &pm->cmd)/*(pm->cmd.buttons&BUTTON_FORCE_FOCUS)*/ )//Holding focus
				{//if no special worked, do nothing
					return LS_NONE;
				}
			}
			//checked all special attacks, if we're in a parry, attack from that move
			saberMoveName_t parryAttackMove = PM_CheckPlayerAttackFromParry( curmove );
			if ( parryAttackMove != LS_NONE )
			{
				return parryAttackMove;
			}
			//check regular attacks
			if ( forwardmove > 0 )
			{//forward left = TR2BL slash
				return LS_A_TR2BL;
			}
			else if ( forwardmove < 0 )
			{//backward left = BR2TL uppercut
				return LS_A_BR2TL;
			}
			else
			{//just left is a right slice
				return LS_A_R2L;
			}
		}
	}
	else
	{//not moving left or right
		if ( forwardmove > 0 )
		{//forward= T2B slash
			saberMoveName_t stabDownMove = noSpecials?LS_NONE:PM_CheckStabDown();
			if ( stabDownMove != LS_NONE )
			{
				return stabDownMove;
			}
			if ( ((pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )//player in third person, not zoomed in
			{//player in thirdperson, not zoomed in
				//flip-over attack logic
				if ( !noSpecials && PM_CheckFlipOverAttackMove( qfalse ) )
				{//flip over-forward down-attack
					return PM_SaberFlipOverAttackMove();
				}
				//lunge attack logic
				else if ( PM_CheckLungeAttackMove() )
				{
					return PM_SaberLungeAttackMove( qtrue );
				}
				//jump forward attack logic
				else if ( !noSpecials && PM_CheckJumpForwardAttackMove() )
				{
					return PM_SaberJumpForwardAttackMove();
				}
			}

			//player NPC with enemy: autoMove logic
			if ( pm->gent
				&& pm->gent->enemy
				&& pm->gent->enemy->client )
			{//I have an active enemy
				if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() )
				{//a player who is running at an enemy
					//if the enemy is not a jedi, don't use top-down, pick a diagonal or side attack
					if ( pm->gent->enemy->s.weapon != WP_SABER
						&& pm->gent->enemy->client->NPC_class != CLASS_REMOTE//too small to do auto-aiming accurately
						&& pm->gent->enemy->client->NPC_class != CLASS_SEEKER//too small to do auto-aiming accurately
						&& pm->gent->enemy->client->NPC_class != CLASS_GONK//too short to do auto-aiming accurately
						&& pm->gent->enemy->client->NPC_class != CLASS_HOWLER//too short to do auto-aiming accurately
						&& g_saberAutoAim->integer )
					{
						saberMoveName_t autoMove = PM_AttackForEnemyPos( qfalse, (qboolean)(pm->ps->clientNum>=MAX_CLIENTS&&!PM_ControlledByPlayer()) );
						if ( autoMove != LS_INVALID )
						{
							return autoMove;
						}
					}
				}

				if ( pm->ps->clientNum>=MAX_CLIENTS && !PM_ControlledByPlayer() ) //NPC ONLY
				{//NPC
					if ( PM_CheckFlipOverAttackMove( qtrue ) )
					{
						return PM_SaberFlipOverAttackMove();
					}
				}
			}

			//Regular NPCs
			if ( pm->ps->clientNum >= MAX_CLIENTS && !PM_ControlledByPlayer() ) //NPC ONLY
			{//NPC or player in third person, not zoomed in
				//fwd jump attack logic
				if ( PM_CheckJumpForwardAttackMove() )
				{
					return PM_SaberJumpForwardAttackMove();
				}
				//lunge attack logic
				else if ( PM_CheckLungeAttackMove() )
				{
					return PM_SaberLungeAttackMove( qtrue );
				}
			}

			if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
			{//player
				if ( G_TryingSpecial(pm->gent,&pm->cmd) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
				{//if no special worked, do nothing
					return LS_NONE;
				}
			}

			//checked all special attacks, if we're in a parry, attack from that move
			saberMoveName_t parryAttackMove = PM_CheckPlayerAttackFromParry( curmove );
			if ( parryAttackMove != LS_NONE )
			{
				return parryAttackMove;
			}
			//check regular attacks
			return LS_A_T2B;
		}
		else if ( forwardmove < 0 )
		{//backward= T2B slash//B2T uppercut?
			if ( g_saberNewControlScheme->integer )
			{
				saberMoveName_t pullAtk = PM_CheckPullAttack();
				if ( pullAtk != LS_NONE )
				{
					return pullAtk;
				}
			}

			if ( g_saberNewControlScheme->integer
				&& (pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer())  //PLAYER ONLY
				&& (pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus, trying special backwards attacks
			{//player lunge attack logic
				if ( ( pm->ps->dualSabers //or dual
						|| pm->ps->saberAnimLevel == SS_STAFF )//pm->ps->SaberStaff() )//or staff
					&& G_EnoughPowerForSpecialMove( pm->ps->forcePower, SABER_ALT_ATTACK_POWER_FB )/*pm->ps->forcePower >= SABER_ALT_ATTACK_POWER_FB*/ )//have enough force power to pull it off
				{//alt+back+attack using fast, dual or staff attacks
					PM_SaberLungeAttackMove( qfalse );
				}
			}
			else if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) //NPC
				|| ((pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )//player in third person, not zooomed
			{//NPC or player in third person, not zoomed
				if ( PM_CheckBackflipAttackMove() )
				{
					return PM_SaberBackflipAttackMove();//backflip attack
				}
				if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
				{//player
					if ( G_TryingSpecial(pm->gent,&pm->cmd) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
					{//if no special worked, do nothing
						return LS_NONE;
					}
				}
				//if ( !PM_InKnockDown( pm->ps ) )
				//check backstabs
				if ( !(pm->ps->saber[0].saberFlags&SFL_NO_BACK_ATTACK)
					&& (!pm->ps->dualSabers || !(pm->ps->saber[1].saberFlags&SFL_NO_BACK_ATTACK)) )
				{//okay to do backstabs with this saber
					if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
					{//only when on ground
						if ( pm->gent && pm->gent->enemy )
						{//FIXME: or just trace for a valid enemy standing behind me?  And no enemy in front?
							vec3_t enemyDir, faceFwd, facingAngles = {0, pm->ps->viewangles[YAW], 0};
							AngleVectors( facingAngles, faceFwd, NULL, NULL );
							VectorSubtract( pm->gent->enemy->currentOrigin, pm->ps->origin, enemyDir );
							float dot = DotProduct( enemyDir, faceFwd );
							if ( dot < 0 )
							{//enemy is behind me
								if ( dot < -0.75f
									&& DistanceSquared( pm->gent->currentOrigin, pm->gent->enemy->currentOrigin ) < 16384//128 squared
									&& (pm->ps->saberAnimLevel == SS_FAST || pm->ps->saberAnimLevel == SS_STAFF || (pm->gent->client &&(pm->gent->client->NPC_class == CLASS_TAVION||pm->gent->client->NPC_class == CLASS_ALORA)&&Q_irand(0,1))) )
								{//fast attacks and Tavion
									if ( !(pm->ps->pm_flags&PMF_DUCKED) && pm->cmd.upmove >= 0 )
									{//can't do it while ducked?
										if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) || (pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG) )
										{//only fencers and above can do this
											return LS_A_BACKSTAB;
										}
									}
								}
								else if ( pm->ps->saberAnimLevel != SS_FAST
									&& pm->ps->saberAnimLevel != SS_STAFF )
								{//medium and higher attacks
									if ( (pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0 )
									{
										return LS_A_BACK_CR;
									}
									else
									{
										return LS_A_BACK;
									}
								}
							}
							else
							{//enemy in front
								float enemyDistSq = DistanceSquared( pm->gent->currentOrigin, pm->gent->enemy->currentOrigin );
								if ( ((pm->ps->saberAnimLevel == FORCE_LEVEL_1 ||
										pm->ps->saberAnimLevel == SS_STAFF ||
										pm->gent->client->NPC_class == CLASS_TAVION ||
										pm->gent->client->NPC_class == CLASS_ALORA ||
										(pm->gent->client->NPC_class == CLASS_DESANN && !Q_irand(0,3))) &&
									enemyDistSq > 16384) ||
									pm->gent->enemy->health <= 0 )//128 squared
								{//my enemy is pretty far in front of me and I'm using fast attacks
									if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) ||
										( pm->gent && pm->gent->client && pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG && Q_irand( 0, pm->gent->NPC->rank ) > RANK_ENSIGN ) )
									{//only fencers and higher can do this, higher rank does it more
										if ( PM_CheckEnemyInBack( 128 ) )
										{
											return PM_PickBackStab();
										}
									}
								}
								else if ( ((pm->ps->saberAnimLevel >= FORCE_LEVEL_2 || pm->gent->client->NPC_class == CLASS_DESANN) && enemyDistSq > 40000) || pm->gent->enemy->health <= 0 )//200 squared
								{//enemy is very faw away and I'm using medium/strong attacks
									if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) ||
										( pm->gent && pm->gent->client && pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG && Q_irand( 0, pm->gent->NPC->rank ) > RANK_ENSIGN ) )
									{//only fencers and higher can do this, higher rank does it more
										if ( PM_CheckEnemyInBack( 164 ) )
										{
											return PM_PickBackStab();
										}
									}
								}
							}
						}
						else
						{//no current enemy
							if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->gent && pm->gent->client )
							{//only player
								if ( PM_CheckEnemyInBack( 128 ) )
								{
									return PM_PickBackStab();
								}
							}
						}
					}
				}
			}

			if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
			{//player
				if ( G_TryingSpecial( pm->gent, &pm->cmd ) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
				{//if no special worked, do nothing
					return LS_NONE;
				}
			}

			//checked all special attacks, if we're in a parry, attack from that move
			saberMoveName_t parryAttackMove = PM_CheckPlayerAttackFromParry( curmove );
			if ( parryAttackMove != LS_NONE )
			{
				return parryAttackMove;
			}
			//check regular attacks
			//else just swing down
			return LS_A_T2B;
		}
		else
		{//not moving in any direction
			if ( PM_SaberInBounce( curmove ) )
			{//bounces should go to their default attack if you don't specify a direction but are attacking
				if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
				{//player
					if ( G_TryingSpecial(pm->gent,&pm->cmd) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
					{//if no special worked, do nothing
						return LS_NONE;
					}
				}
				saberMoveName_t newmove;
				if ( pm->ps->clientNum && !PM_ControlledByPlayer() && Q_irand( 0, 3 ) )
				{//use NPC random
					newmove = PM_NPCSaberAttackFromQuad( saberMoveData[curmove].endQuad );
				}
				else
				{//player uses chain-attack
					newmove = saberMoveData[curmove].chain_attack;
				}
				if ( PM_SaberKataDone( curmove, newmove ) )
				{
					return saberMoveData[curmove].chain_idle;
				}
				else
				{
					return newmove;
				}
			}
			else if ( PM_SaberInKnockaway( curmove ) )
			{//bounces should go to their default attack if you don't specify a direction but are attacking
				if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
				{//player
					if ( G_TryingSpecial( pm->gent, &pm->cmd ) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
					{//if no special worked, do nothing
						return LS_NONE;
					}
				}
				saberMoveName_t newmove;
				if ( pm->ps->clientNum && !PM_ControlledByPlayer() && Q_irand( 0, 3 ) )
				{//use NPC random
					newmove = PM_NPCSaberAttackFromQuad( saberMoveData[curmove].endQuad );
				}
				else
				{
					if ( pm->ps->saberAnimLevel == SS_FAST ||
						pm->ps->saberAnimLevel == SS_TAVION )
					{//player is in fast attacks, so come right back down from the same spot
						newmove = PM_AttackMoveForQuad( saberMoveData[curmove].endQuad );
					}
					else
					{//use a transition to wrap to another attack from a different dir
						newmove = saberMoveData[curmove].chain_attack;
					}
				}
				if ( PM_SaberKataDone( curmove, newmove ) )
				{
					return saberMoveData[curmove].chain_idle;
				}
				else
				{
					return newmove;
				}
			}
			else if ( curmove == LS_READY
				|| curmove == LS_A_FLIP_STAB
				|| curmove == LS_A_FLIP_SLASH
				|| ( curmove >= LS_PARRY_UP
					&& curmove <= LS_REFLECT_LL ) )
			{//Not moving at all, not too busy to attack
				//push + lookdown + attack + dual sabers = LS_DUAL_SPIN_PROTECT
				if ( g_saberNewControlScheme->integer )
				{
					if ( PM_CheckDualSpinProtect() )
					{
						return LS_DUAL_SPIN_PROTECT;
					}
					if ( PM_CheckStaffKata() )
					{
						return LS_STAFF_SOULCAL;
					}
				}
				if ( pm->ps->clientNum < MAX_CLIENTS || PM_ControlledByPlayer() ) //PLAYER ONLY
				{//player
					if ( G_TryingSpecial( pm->gent, &pm->cmd ) )//(pm->cmd.buttons&BUTTON_FORCE_FOCUS) )//Holding focus
					{//if no special worked, do nothing
						return LS_NONE;
					}
				}
				//checked all special attacks, if we're in a parry, attack from that move
				saberMoveName_t parryAttackMove = PM_CheckPlayerAttackFromParry( curmove );
				if ( parryAttackMove != LS_NONE )
				{
					return parryAttackMove;
				}
				//check regular attacks
				if ( pm->ps->clientNum || g_saberAutoAim->integer )
				{//auto-aim
					if ( pm->gent && pm->gent->enemy )
					{//based on enemy position, pick a proper attack
						saberMoveName_t autoMove = PM_AttackForEnemyPos( qtrue, (qboolean)(pm->ps->clientNum>=MAX_CLIENTS) );
						if ( autoMove != LS_INVALID )
						{
							return autoMove;
						}
					}
					else if ( fabs(pm->ps->viewangles[0]) > 30 )
					{//looking far up or far down uses the top to bottom attack, presuming you want a vertical attack
						return LS_A_T2B;
					}
				}
				else
				{//for now, just pick a random attack
					return ((saberMoveName_t)Q_irand( LS_A_TL2BR, LS_A_T2B ));
				}
			}
		}
	}
	//FIXME: pick a return?
	return LS_NONE;
}

saberMoveName_t PM_SaberAnimTransitionMove( saberMoveName_t curmove, saberMoveName_t newmove )
{
	//FIXME: take FP_SABER_OFFENSE into account here somehow?
	int retmove = newmove;
	if ( curmove == LS_READY )
	{//just standing there
		switch ( newmove )
		{
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			//transition is the start
			retmove = LS_S_TL2BR + (newmove-LS_A_TL2BR);
			break;
		default:
			break;
		}
	}
	else
	{
		switch ( newmove )
		{
		//transitioning to ready pose
		case LS_READY:
			switch ( curmove )
			{
			//transitioning from an attack
			case LS_A_TL2BR:
			case LS_A_L2R:
			case LS_A_BL2TR:
			case LS_A_BR2TL:
			case LS_A_R2L:
			case LS_A_TR2BL:
			case LS_A_T2B:
				//transition is the return
				retmove = LS_R_TL2BR + (newmove-LS_A_TL2BR);
				break;
			default:
				break;
			}
			break;
		//transitioning to an attack
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			if ( newmove == curmove )
			{//FIXME: need a spin or something or go to next level, but for now, just play the return
				//going into another attack...
				//allow endless chaining in level 1 attacks, several in level 2 and only one or a few in level 3
				//FIXME: don't let strong attacks chain to an attack in the opposite direction ( > 45 degrees?)
				if ( PM_SaberKataDone( curmove, newmove ) )
				{//done with this kata, must return to ready before attack again
					retmove = LS_R_TL2BR + (newmove-LS_A_TL2BR);
				}
				else
				{//okay to chain to another attack
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
				}
			}
			else if ( saberMoveData[curmove].endQuad == saberMoveData[newmove].startQuad )
			{//new move starts from same quadrant
				retmove = newmove;
			}
			else
			{
				switch ( curmove )
				{
				//transitioning from an attack
				case LS_A_TL2BR:
				case LS_A_L2R:
				case LS_A_BL2TR:
				case LS_A_BR2TL:
				case LS_A_R2L:
				case LS_A_TR2BL:
				case LS_A_T2B:
				case LS_D1_BR:
				case LS_D1__R:
				case LS_D1_TR:
				case LS_D1_T_:
				case LS_D1_TL:
				case LS_D1__L:
				case LS_D1_BL:
				case LS_D1_B_:
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
					break;
				//transitioning from a return
				case LS_R_TL2BR:
				case LS_R_L2R:
				case LS_R_BL2TR:
				case LS_R_BR2TL:
				case LS_R_R2L:
				case LS_R_TR2BL:
				case LS_R_T2B:
				//transitioning from a bounce
				/*
				case LS_BOUNCE_UL2LL:
				case LS_BOUNCE_LL2UL:
				case LS_BOUNCE_L2LL:
				case LS_BOUNCE_L2UL:
				case LS_BOUNCE_UR2LR:
				case LS_BOUNCE_LR2UR:
				case LS_BOUNCE_R2LR:
				case LS_BOUNCE_R2UR:
				case LS_BOUNCE_TOP:
				case LS_OVER_UR2UL:
				case LS_OVER_UL2UR:
				case LS_BOUNCE_UR:
				case LS_BOUNCE_UL:
				case LS_BOUNCE_LR:
				case LS_BOUNCE_LL:
				*/
				//transitioning from a parry/reflection/knockaway/broken parry
				case LS_PARRY_UP:
				case LS_PARRY_UR:
				case LS_PARRY_UL:
				case LS_PARRY_LR:
				case LS_PARRY_LL:
				case LS_REFLECT_UP:
				case LS_REFLECT_UR:
				case LS_REFLECT_UL:
				case LS_REFLECT_LR:
				case LS_REFLECT_LL:
				case LS_K1_T_:
				case LS_K1_TR:
				case LS_K1_TL:
				case LS_K1_BR:
				case LS_K1_BL:
				case LS_V1_BR:
				case LS_V1__R:
				case LS_V1_TR:
				case LS_V1_T_:
				case LS_V1_TL:
				case LS_V1__L:
				case LS_V1_BL:
				case LS_V1_B_:
				case LS_H1_T_:
				case LS_H1_TR:
				case LS_H1_TL:
				case LS_H1_BR:
				case LS_H1_BL:
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
					break;
				//NB: transitioning from transitions is fine
				default:
					break;
				}
			}
			break;
		//transitioning to any other anim is not supported
		default:
			break;
		}
	}

	if ( retmove == LS_NONE )
	{
		return newmove;
	}

	return ((saberMoveName_t)retmove);
}

/*
-------------------------
PM_LegsAnimForFrame
Returns animNumber for current frame
-------------------------
*/
int PM_LegsAnimForFrame( gentity_t *ent, int legsFrame )
{
	//Must be a valid client
	if ( ent->client == NULL )
		return -1;

	//Must have a file index entry
	if( ValidAnimFileIndex( ent->client->clientInfo.animFileIndex ) == qfalse )
		return -1;

	animation_t *animations = level.knownAnimFileSets[ent->client->clientInfo.animFileIndex].animations;
	int	glaIndex = gi.G2API_GetAnimIndex(&(ent->ghoul2[0]));

	for ( int animation = 0; animation < BOTH_CIN_1; animation++ )	//first anim after last legs
	{
		if ( animation >= TORSO_DROPWEAP1 && animation < LEGS_TURN1 )	//first legs only anim
		{//not a possible legs anim
			continue;
		}

		if ( animations[animation].glaIndex != glaIndex )
		{
			continue;
		}

		if ( animations[animation].firstFrame > legsFrame )
		{//This anim starts after this frame
			continue;
		}

		if ( animations[animation].firstFrame + animations[animation].numFrames < legsFrame )
		{//This anim ends before this frame
			continue;
		}
		//else, must be in this anim!
		return animation;
	}

	//Not in ANY torsoAnim?  SHOULD NEVER HAPPEN
//	assert(0);
	return -1;
}

int PM_ValidateAnimRange( const int startFrame, const int endFrame, const float animSpeed )
{//given a startframe and endframe, see if that lines up with any known animation
	animation_t *animations = level.knownAnimFileSets[0].animations;

	for ( int anim = 0; anim < MAX_ANIMATIONS; anim++ )
	{
		if ( animSpeed < 0 )
		{//playing backwards
			 if ( animations[anim].firstFrame == endFrame )
			 {
				if ( animations[anim].numFrames + animations[anim].firstFrame == startFrame )
				{
					//Com_Printf( "valid reverse anim: %s\n", animTable[anim].name );
					return anim;
				}
			 }
		}
		else
		{//playing forwards
			if ( animations[anim].firstFrame == startFrame )
			{//This anim starts on this frame
				if ( animations[anim].firstFrame + animations[anim].numFrames == endFrame )
				{//This anim ends on this frame
					//Com_Printf( "valid forward anim: %s\n", animTable[anim].name );
					return anim;
				}
			}
		}
		//else, must not be this anim!
	}

	//Not in ANY anim?  SHOULD NEVER HAPPEN
	Com_Printf( "invalid anim range %d to %d, speed %4.2f\n", startFrame, endFrame, animSpeed );
	return -1;
}
/*
-------------------------
PM_TorsoAnimForFrame
Returns animNumber for current frame
-------------------------
*/
int PM_TorsoAnimForFrame( gentity_t *ent, int torsoFrame )
{
	//Must be a valid client
	if ( ent->client == NULL )
		return -1;

	//Must have a file index entry
	if( ValidAnimFileIndex( ent->client->clientInfo.animFileIndex ) == qfalse )
		return -1;

	animation_t *animations = level.knownAnimFileSets[ent->client->clientInfo.animFileIndex].animations;
	int	glaIndex = gi.G2API_GetAnimIndex(&(ent->ghoul2[0]));

	for ( int animation = 0; animation < LEGS_TURN1; animation++ )	//first legs only anim
	{
		if ( animations[animation].glaIndex != glaIndex )
		{
			continue;
		}

		if ( animations[animation].firstFrame > torsoFrame )
		{//This anim starts after this frame
			continue;
		}

		if ( animations[animation].firstFrame + animations[animation].numFrames < torsoFrame )
		{//This anim ends before this frame
			continue;
		}
		//else, must be in this anim!
		return animation;
	}

	//Not in ANY torsoAnim?  SHOULD NEVER HAPPEN
//	assert(0);
	return -1;
}

qboolean PM_FinishedCurrentLegsAnim( gentity_t *self )
{
	int		junk, curFrame;
	float	currentFrame, animSpeed;

	if ( !self->client )
	{
		return qtrue;
	}

	gi.G2API_GetBoneAnimIndex( &self->ghoul2[self->playerModel], self->rootBone, (cg.time?cg.time:level.time), &currentFrame, &junk, &junk, &junk, &animSpeed, NULL );
	curFrame = floor( currentFrame );

	int				legsAnim	= self->client->ps.legsAnim;
	animation_t		*animations	= level.knownAnimFileSets[self->client->clientInfo.animFileIndex].animations;

	if ( curFrame >= animations[legsAnim].firstFrame + (animations[legsAnim].numFrames - 2) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
PM_HasAnimation
-------------------------
*/

qboolean PM_HasAnimation( gentity_t *ent, int animation )
{
	//Must be a valid client
	if ( !ent || ent->client == NULL )
		return qfalse;

	//must be a valid anim number
	if ( animation < 0 || animation >= MAX_ANIMATIONS )
	{
		return qfalse;
	}
	//Must have a file index entry
	if( ValidAnimFileIndex( ent->client->clientInfo.animFileIndex ) == qfalse )
		return qfalse;

	animation_t *animations = level.knownAnimFileSets[ent->client->clientInfo.animFileIndex].animations;

	//No frames, no anim
	if ( animations[animation].numFrames == 0 )
		return qfalse;

	//Has the sequence
	return qtrue;
}

int PM_PickAnim( gentity_t *self, int minAnim, int maxAnim )
{
	int anim;
	int count = 0;

	if ( !self )
	{
		return Q_irand(minAnim, maxAnim);
	}

	do
	{
		anim = Q_irand(minAnim, maxAnim);
		count++;
	}
	while ( !PM_HasAnimation( self, anim ) && count < 1000 );

	return anim;
}

/*
-------------------------
PM_AnimLength
-------------------------
*/

int PM_AnimLength( int index, animNumber_t anim )
{
	if ( ValidAnimFileIndex( index ) == false )
		return 0;

	return level.knownAnimFileSets[index].animations[anim].numFrames * abs(level.knownAnimFileSets[index].animations[anim].frameLerp);
}

/*
-------------------------
PM_SetLegsAnimTimer
-------------------------
*/

void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time )
{
	*legsAnimTimer = time;

	if ( *legsAnimTimer < 0 && time != -1 )
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional
		*legsAnimTimer = 0;
	}

	if ( !*legsAnimTimer && ent && Q3_TaskIDPending( ent, TID_ANIM_LOWER ) )
	{//Waiting for legsAnimTimer to complete, and it just got set to zero
		if ( !Q3_TaskIDPending( ent, TID_ANIM_BOTH) )
		{//Not waiting for top
			Q3_TaskIDComplete( ent, TID_ANIM_LOWER );
		}
		else
		{//Waiting for both to finish before complete
			Q3_TaskIDClear( &ent->taskID[TID_ANIM_LOWER] );//Bottom is done, regardless
			if ( !Q3_TaskIDPending( ent, TID_ANIM_UPPER) )
			{//top is done and we're done
				Q3_TaskIDComplete( ent, TID_ANIM_BOTH );
			}
		}
	}
}

/*
-------------------------
PM_SetTorsoAnimTimer
-------------------------
*/

void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time )
{
	*torsoAnimTimer = time;

	if ( *torsoAnimTimer < 0 && time != -1 )
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional
		*torsoAnimTimer = 0;
	}

	if ( !*torsoAnimTimer && ent && Q3_TaskIDPending( ent, TID_ANIM_UPPER ) )
	{//Waiting for torsoAnimTimer to complete, and it just got set to zero
		if ( !Q3_TaskIDPending( ent, TID_ANIM_BOTH) )
		{//Not waiting for bottom
			Q3_TaskIDComplete( ent, TID_ANIM_UPPER );
		}
		else
		{//Waiting for both to finish before complete
			Q3_TaskIDClear( &ent->taskID[TID_ANIM_UPPER] );//Top is done, regardless
			if ( !Q3_TaskIDPending( ent, TID_ANIM_LOWER) )
			{//lower is done and we're done
				Q3_TaskIDComplete( ent, TID_ANIM_BOTH );
			}
		}
	}
}

extern qboolean PM_SpinningSaberAnim( int anim );
extern float saberAnimSpeedMod[NUM_FORCE_POWER_LEVELS];
void PM_SaberStartTransAnim( int saberAnimLevel, int anim, float *animSpeed, gentity_t *gent )
{
	if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_ROLL_STAB )
	{
		if ( g_saberAnimSpeed->value != 1.0f )
		{
			*animSpeed *= g_saberAnimSpeed->value;
		}
		else if ( gent && gent->client && gent->client->ps.weapon == WP_SABER )
		{
			if ( gent->client->ps.saber[0].animSpeedScale != 1.0f )
			{
				*animSpeed *= gent->client->ps.saber[0].animSpeedScale;
			}
			if ( gent->client->ps.dualSabers
				&& gent->client->ps.saber[1].animSpeedScale != 1.0f )
			{
				*animSpeed *= gent->client->ps.saber[1].animSpeedScale;
			}
		}
	}
	if ( gent
		&& gent->client
		&& gent->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER)
		&& gent->client->ps.dualSabers
		&& saberAnimLevel == SS_DUAL
		&& gent->weaponModel[1] )
	{//using a scepter and dual style, slow down anims
		if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_H7_S7_BR )
		{
			*animSpeed *= 0.75;
		}
	}
	if ( gent && gent->client && gent->client->ps.forceRageRecoveryTime > level.time )
	{//rage recovery
		if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_H1_S1_BR )
		{//animate slower
			*animSpeed *= 0.75;
		}
	}
	else if ( gent && gent->NPC && gent->NPC->rank == RANK_CIVILIAN )
	{//grunt reborn
		if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_R1_TR_S1 )
		{//his fast attacks are slower
			if ( !PM_SpinningSaberAnim( anim ) )
			{
				*animSpeed *= 0.75;
			}
			return;
		}
	}
	else if ( gent && gent->client )
	{
		if ( gent->client->ps.saber[0].type == SABER_LANCE || gent->client->ps.saber[0].type == SABER_TRIDENT )
		{//FIXME: hack for now - these use the fast anims, but slowed down.  Should have own style
			if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_R1_TR_S1 )
			{//his fast attacks are slower
				if ( !PM_SpinningSaberAnim( anim ) )
				{
					*animSpeed *= 0.75;
				}
				return;
			}
		}
	}

	if ( ( anim >= BOTH_T1_BR__R &&
		anim <= BOTH_T1_BL_TL ) ||
		( anim >= BOTH_T3_BR__R &&
		anim <= BOTH_T3_BL_TL ) ||
		( anim >= BOTH_T5_BR__R &&
		anim <= BOTH_T5_BL_TL ) )
	{
		if ( saberAnimLevel == FORCE_LEVEL_1 || saberAnimLevel == FORCE_LEVEL_5 )
		{//FIXME: should not be necc for FORCE_LEVEL_1's
			*animSpeed *= 1.5;
		}
		else if ( saberAnimLevel == FORCE_LEVEL_3 )
		{
			*animSpeed *= 0.75;
		}
	}
}
/*
void PM_SaberStartTransAnim( int anim, int entNum, int saberOffenseLevel, float *animSpeed )
{
	//check starts
	if ( ( anim >= BOTH_S1_S1_T_ &&
		anim <= BOTH_S1_S1_TR ) ||
		( anim >= BOTH_S1_S1_T_ &&
			anim <= BOTH_S1_S1_TR ) ||
		( anim >= BOTH_S3_S1_T_ &&
			anim <= BOTH_S3_S1_TR ) )
	{
		if ( entNum == 0 )
		{
			*animSpeed *= saberAnimSpeedMod[FORCE_LEVEL_3];
		}
		else
		{
			*animSpeed *= saberAnimSpeedMod[saberOffenseLevel];
		}
	}
	//Check transitions
	else if ( PM_SpinningSaberAnim( anim ) )
	{//spins stay normal speed
		return;
	}
	else if ( ( anim >= BOTH_T1_BR__R &&
			anim <= BOTH_T1_BL_TL ) ||
			( anim >= BOTH_T2_BR__R &&
			anim <= BOTH_T2_BL_TL ) ||
			( anim >= BOTH_T3_BR__R &&
			anim <= BOTH_T3_BL_TL ) )
	{//slow down the transitions
		if ( entNum == 0 && saberOffenseLevel <= FORCE_LEVEL_2 )
		{
			*animSpeed *= saberAnimSpeedMod[saberOffenseLevel];
		}
		else
		{
			*animSpeed *= saberAnimSpeedMod[saberOffenseLevel]/2.0f;
		}
	}

	return;
}
*/
extern qboolean		player_locked;
extern qboolean		MatrixMode;
float PM_GetTimeScaleMod( gentity_t *gent )
{
	if ( g_timescale->value )
	{
		if ( !MatrixMode
			&& gent->client->ps.legsAnim != BOTH_FORCELONGLEAP_START
			&& gent->client->ps.legsAnim != BOTH_FORCELONGLEAP_ATTACK
			&& gent->client->ps.legsAnim != BOTH_FORCELONGLEAP_LAND )
		{
			if ( gent && gent->s.clientNum == 0 && !player_locked && gent->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				return (1.0 / g_timescale->value);
			}
			else if ( gent && gent->client && gent->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				return (1.0 / g_timescale->value);
			}
		}
	}
	return 1.0f;
}

static inline qboolean PM_IsHumanoid( CGhoul2Info *ghlInfo )
{
	char	*GLAName;
	GLAName = gi.G2API_GetGLAName( ghlInfo );
	assert(GLAName);

	if ( !Q_stricmp( "models/players/_humanoid/_humanoid", GLAName ) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
PM_SetAnimFinal
-------------------------
*/
#define G2_DEBUG_TIMING (0)
void PM_SetAnimFinal(int *torsoAnim,int *legsAnim,
					 int setAnimParts,int anim,int setAnimFlags,
					 int *torsoAnimTimer,int *legsAnimTimer,
					 gentity_t *gent,int blendTime)		// default blendTime=350
{

// BASIC SETUP AND SAFETY CHECKING
//=================================

	// If It Is A Busted Entity, Don't Do Anything Here.
	//---------------------------------------------------
	if (!gent || !gent->client)
	{
		return;
	}

	// Make Sure This Character Has Such An Anim And A Model
	//-------------------------------------------------------
	if (anim<0 || anim>=MAX_ANIMATIONS || !ValidAnimFileIndex(gent->client->clientInfo.animFileIndex))
	{
		#ifndef FINAL_BUILD
 		if (g_AnimWarning->integer)
		{
			if (anim<0 || anim>=MAX_ANIMATIONS)
			{
				gi.Printf(S_COLOR_RED"PM_SetAnimFinal: Invalid Anim Index (%d)!\n", anim);
			}
			else
			{
				gi.Printf(S_COLOR_RED"PM_SetAnimFinal: Invalid Anim File Index (%d)!\n", gent->client->clientInfo.animFileIndex);
			}
		}
		#endif
		return;
	}


	// Get Global Time Properties
	//----------------------------
	float			timeScaleMod  = PM_GetTimeScaleMod( gent );
	const int		actualTime	  = (cg.time?cg.time:level.time);
	const animation_t*	animations	  = level.knownAnimFileSets[gent->client->clientInfo.animFileIndex].animations;
	const animation_t&	curAnim		  = animations[anim];

	// Make Sure This Character Has Such An Anim And A Model
	//-------------------------------------------------------
	if (animations[anim].numFrames==0)
	{
	#ifndef FINAL_BUILD
		static int	LastAnimWarningNum=0;
		if (LastAnimWarningNum!=anim)
		{
			if ((cg_debugAnim.integer==3)	||												// 3 = do everyone
 				(cg_debugAnim.integer==1 && gent->s.number==0) ||							// 1 = only the player
				(cg_debugAnim.integer==2 && gent->s.number!=0) ||							// 2 = only everyone else
				(cg_debugAnim.integer==4 && gent->s.number!=cg_debugAnimTarget.integer) 	// 4 = specific entnum
				)
			{
				gi.Printf(S_COLOR_RED"PM_SetAnimFinal: Anim %s does not exist in this model (%s)!\n", animTable[anim].name, gent->NPC_type );
			}
		}
		LastAnimWarningNum = anim;
	#endif
		return;
	}

	// If It's Not A Ghoul 2 Model, Just Remember The Anims And Stop, Because Everything Beyond This Is Ghoul2
	//---------------------------------------------------------------------------------------------------------
	if (!gi.G2API_HaveWeGhoul2Models(gent->ghoul2))
	{
		if (setAnimParts&SETANIM_TORSO)
		{
			(*torsoAnim) = anim;
		}
		if (setAnimParts&SETANIM_LEGS)
		{
			(*legsAnim) = anim;
		}
		return;
	}


	// Lower Offensive Skill Slows Down The Saber Start Attack Animations
	//--------------------------------------------------------------------
	PM_SaberStartTransAnim( gent->client->ps.saberAnimLevel, anim, &timeScaleMod, gent );



// SETUP VALUES FOR INCOMMING ANIMATION
//======================================
	const bool	animFootMove  = (PM_WalkingAnim(anim) || PM_RunningAnim(anim) || anim==BOTH_CROUCH1WALK || anim==BOTH_CROUCH1WALKBACK);
	const bool	animHoldless  = (setAnimFlags&SETANIM_FLAG_HOLDLESS)!=0;
	const bool	animHold	  = (setAnimFlags&SETANIM_FLAG_HOLD)!=0;
	const bool	animRestart	  = (setAnimFlags&SETANIM_FLAG_RESTART)!=0;
	const bool	animOverride  = (setAnimFlags&SETANIM_FLAG_OVERRIDE)!=0;
	const bool	animSync	  = (g_synchSplitAnims->integer!=0 && !animRestart);
	float	animCurrent	  = (-1.0f);
	float	animSpeed	  = (50.0f / curAnim.frameLerp * timeScaleMod); // animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
	const float	animFPS		  = (fabsf(curAnim.frameLerp));
	const int		animDurMSec	  = (int)(((curAnim.numFrames - 1) * animFPS) / timeScaleMod);
	const int		animHoldMSec  = ((animHoldless && timeScaleMod==1.0f)?((animDurMSec>1)?(animDurMSec-1):(animFPS)):(animDurMSec));
	int		animFlags	  = (curAnim.loopFrames!=-1)?(BONE_ANIM_OVERRIDE_LOOP):(BONE_ANIM_OVERRIDE_FREEZE);
	int		animStart	  = (curAnim.firstFrame);
	int		animEnd		  = (curAnim.firstFrame)+(animations[anim].numFrames);

	// If We Have A Blend Timer, Add The Blend Flag
	//----------------------------------------------
	if (blendTime > 0)
	{
		animFlags |= BONE_ANIM_BLEND;
	}

	// If Animation Is Going Backwards, Swap Last And First Frames
	//-------------------------------------------------------------
	if (animSpeed<0.0f)
	{
//	#ifndef FINAL_BUILD
	#if 0
		if (g_AnimWarning->integer==1)
		{
			if (animFlags&BONE_ANIM_OVERRIDE_LOOP)
			{
				gi.Printf(S_COLOR_YELLOW"PM_SetAnimFinal: WARNING: Anim (%s) looping backwards!\n", animTable[anim].name);
			}
		}
	#endif

		int temp	= animEnd;
		animEnd		= animStart;
		animStart	= temp;
		blendTime	= 0;
	}

	// If The Animation Is Walking Or Running, Attempt To Scale The Playback Speed To Match
	//--------------------------------------------------------------------------------------
	if (g_noFootSlide->integer
		&& animFootMove
		&& !(animSpeed<0.0f)
		//FIXME: either read speed from animation.cfg or only do this for NPCs
		//			for whom we've specifically determined the proper numbers!
		&& gent->client->NPC_class != CLASS_HOWLER
		&& gent->client->NPC_class != CLASS_WAMPA
		&& gent->client->NPC_class != CLASS_GONK
		&& gent->client->NPC_class != CLASS_HOWLER
		&& gent->client->NPC_class != CLASS_MOUSE
		&& gent->client->NPC_class != CLASS_PROBE
		&& gent->client->NPC_class != CLASS_PROTOCOL
		&& gent->client->NPC_class != CLASS_R2D2
		&& gent->client->NPC_class != CLASS_R5D2
		&& gent->client->NPC_class != CLASS_SEEKER)
	{
		bool	Walking = !!PM_WalkingAnim(anim);
		bool	HasDual = (gent->client->ps.saberAnimLevel==SS_DUAL);
		bool	HasStaff = (gent->client->ps.saberAnimLevel==SS_STAFF);
		float	moveSpeedOfAnim  = 150.0f;//g_noFootSlideRunScale->value;

		if (anim==BOTH_CROUCH1WALK || anim==BOTH_CROUCH1WALKBACK)
		{
			moveSpeedOfAnim = 75.0f;
		}
		else
		{
			if (gent->client->NPC_class == CLASS_HAZARD_TROOPER)
			{
				moveSpeedOfAnim = 50.0f;
			}
			else if (gent->client->NPC_class == CLASS_RANCOR)
			{
				moveSpeedOfAnim = 173.0f;
			}
			else
			{
				if (Walking)
				{
					if (HasDual || HasStaff)
					{
						moveSpeedOfAnim = 100.0f;
					}
					else
					{
						moveSpeedOfAnim = 50.0f;// g_noFootSlideWalkScale->value;
					}
				}
				else
				{
					if (HasStaff)
					{
						moveSpeedOfAnim = 250.0f;
					}
					else
					{
						moveSpeedOfAnim = 150.0f;
					}
				}
			}
		}






		animSpeed *= (gent->resultspeed/moveSpeedOfAnim);
		if (animSpeed<0.01f)
		{
			animSpeed = 0.01f;
		}

		// Make Sure Not To Play Too Fast An Anim
		//----------------------------------------
		float	maxPlaybackSpeed = (1.5f * timeScaleMod);
		if (animSpeed>maxPlaybackSpeed)
		{
			animSpeed = maxPlaybackSpeed;
		}
	}


// GET VALUES FOR EXISTING BODY ANIMATION
//==========================================
	float	bodySpeed	  = 0.0f;
	float	bodyCurrent	  = 0.0f;
	int		bodyStart	  = 0;
	int		bodyEnd		  = 0;
	int		bodyFlags	  = 0;
	int		bodyAnim	  = (*legsAnim);
	int		bodyBone	  = (gent->rootBone);
	bool	bodyTimerOn	  = ((*legsAnimTimer>0) || (*legsAnimTimer)==-1);
	bool	bodyPlay	  = ((setAnimParts&SETANIM_LEGS) && (bodyBone!=-1) && (animOverride || !bodyTimerOn));
	bool	bodyAnimating = !!gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel], bodyBone, actualTime, &bodyCurrent, &bodyStart, &bodyEnd, &bodyFlags, &bodySpeed, NULL);
	bool	bodyOnAnimNow = (bodyAnimating && bodyAnim==anim && bodyStart==animStart && bodyEnd==animEnd);
	bool	bodyMatchTorsFrame = false;


// GET VALUES FOR EXISTING TORSO ANIMATION
//===========================================
	float	torsSpeed	  = 0.0f;
	float	torsCurrent	  = 0.0f;
	int		torsStart	  = 0;
	int		torsEnd		  = 0;
	int		torsFlags	  = 0;
	int		torsAnim	  = (*torsoAnim);
	int		torsBone	  = (gent->lowerLumbarBone);
	bool	torsTimerOn	  = ((*torsoAnimTimer)>0 || (*torsoAnimTimer)==-1);
	bool	torsPlay	  = (gent->client->NPC_class!=CLASS_RANCOR && (setAnimParts&SETANIM_TORSO) && (torsBone!=-1) && (animOverride || !torsTimerOn));
	bool	torsAnimating = !!gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel], torsBone, actualTime, &torsCurrent, &torsStart, &torsEnd, &torsFlags, &torsSpeed, NULL);
	bool	torsOnAnimNow = (torsAnimating && torsAnim==anim && torsStart==animStart && torsEnd==animEnd);
	bool	torsMatchBodyFrame = false;


// APPLY SYNC TO TORSO
//=====================
 	if (animSync && torsPlay && !bodyPlay && bodyOnAnimNow && (!torsOnAnimNow || torsCurrent!=bodyCurrent))
	{
		torsMatchBodyFrame = true;
		animCurrent		=  bodyCurrent;
	}
 	if (animSync && bodyPlay && !torsPlay && torsOnAnimNow && (!bodyOnAnimNow || bodyCurrent!=torsCurrent))
	{
		bodyMatchTorsFrame = true;
		animCurrent		=  torsCurrent;
	}

	// If Already Doing These Exact Parameters, Then Don't Play
	//----------------------------------------------------------
	if (!animRestart)
	{
		torsPlay &= !(torsOnAnimNow && torsSpeed==animSpeed && !torsMatchBodyFrame);
		bodyPlay &= !(bodyOnAnimNow && bodySpeed==animSpeed && !bodyMatchTorsFrame);
	}

#ifndef FINAL_BUILD
	if ((cg_debugAnim.integer==3)	||												// 3 = do everyone
		(cg_debugAnim.integer==1 && gent->s.number==0) ||							// 1 = only the player
		(cg_debugAnim.integer==2 && gent->s.number!=0) ||							// 2 = only everyone else
		(cg_debugAnim.integer==4 && gent->s.number!=cg_debugAnimTarget.integer) 	// 4 = specific entnum
		)
	{
		if (bodyPlay || torsPlay)
		{
			char*	entName = gent->targetname;
			char*	location;

			// Select Entity Name
			//--------------------
			if (!entName || !entName[0])
			{
				entName = gent->NPC_targetname;
			}
			if (!entName || !entName[0])
			{
				entName = gent->NPC_type;
			}
			if (!entName || !entName[0])
			{
				entName = gent->classname;
			}
			if (!entName || !entName[0])
			{
				entName = "UNKNOWN";
			}

			// Select Play Location
			//----------------------
			if (bodyPlay && torsPlay)
			{
				location = "BOTH ";
			}
			else if (bodyPlay)
			{
				location = "LEGS ";
			}
			else
			{
				location = "TORSO";
			}

			// Print It!
			//-----------
	 		Com_Printf("[%10d] ent[%3d-%18s] %s anim[%3d] - %s\n",
	 			actualTime,
				gent->s.number,
				entName,
				location,
				anim,
				animTable[anim].name );
		}
	}
#endif


// PLAY ON THE TORSO
//========================
	if (torsPlay)
	{
		*torsoAnim = anim;
		float oldAnimCurrent = animCurrent;
		if (animCurrent!=bodyCurrent && torsOnAnimNow && !animRestart && !torsMatchBodyFrame)
		{
			animCurrent = torsCurrent;
		}

		gi.G2API_SetAnimIndex(&gent->ghoul2[gent->playerModel], curAnim.glaIndex);
		gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], torsBone,
			animStart,
			animEnd,
			(torsOnAnimNow && !animRestart)?(animFlags&~BONE_ANIM_BLEND):(animFlags),
			animSpeed,
			actualTime,
			animCurrent,
			blendTime);

		if (gent->motionBone!=-1)
		{
			gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->motionBone,
				animStart,
				animEnd,
				(torsOnAnimNow && !animRestart)?(animFlags&~BONE_ANIM_BLEND):(animFlags),
				animSpeed,
				actualTime,
				animCurrent,
				blendTime);
		}

		animCurrent = oldAnimCurrent;

		// If This Animation Is To Be Locked And Held, Calculate The Duration And Set The Timer
		//--------------------------------------------------------------------------------------
		if (animHold || animHoldless)
		{
			PM_SetTorsoAnimTimer(gent, torsoAnimTimer, animHoldMSec);
		}
	}

// PLAY ON THE WHOLE BODY
//========================
	if (bodyPlay)
	{
		*legsAnim = anim;

		if (bodyOnAnimNow && !animRestart && !bodyMatchTorsFrame)
		{
			animCurrent = bodyCurrent;
		}

		gi.G2API_SetAnimIndex(&gent->ghoul2[gent->playerModel], curAnim.glaIndex);
		gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], bodyBone,
			animStart,
			animEnd,
			(bodyOnAnimNow && !animRestart)?(animFlags&~BONE_ANIM_BLEND):(animFlags),
			animSpeed,
			actualTime,
			animCurrent,
			blendTime);

		// If This Animation Is To Be Locked And Held, Calculate The Duration And Set The Timer
		//--------------------------------------------------------------------------------------
		if (animHold || animHoldless)
		{
			PM_SetLegsAnimTimer(gent, legsAnimTimer, animHoldMSec);
		}
	}





// PRINT SOME DEBUG TEXT OF EXISTING VALUES
//==========================================
	if (false)
	{
		gi.Printf("PLAYANIM: (%3d) Speed(%4.2f) ", anim, animSpeed);
		if (bodyAnimating)
		{
			gi.Printf("BODY: (%4.2f) (%4.2f) ", bodyCurrent,  bodySpeed);
		}
		else
		{
			gi.Printf("                      ");
		}
		if (torsAnimating)
		{
			gi.Printf("TORS: (%4.2f) (%4.2f)\n", torsCurrent,  torsSpeed);
		}
		else
		{
			gi.Printf("\n");
		}
	}
}



void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime)
{	// FIXME : once torsoAnim and legsAnim are in the same structure for NPC and Players
	// rename PM_SetAnimFinal to PM_SetAnim and have both NPC and Players call PM_SetAnim

	if ( pm->ps->pm_type >= PM_DEAD )
	{//FIXME: sometimes we'll want to set anims when your dead... twitches, impacts, etc.
		return;
	}

	if ( pm->gent == NULL )
	{
		return;
	}

	if ( !pm->gent || pm->gent->health > 0 )
	{//don't lock anims if the guy is dead
		if ( pm->ps->torsoAnimTimer
			&& PM_LockedAnim( pm->ps->torsoAnim )
			&& !PM_LockedAnim( anim ) )
		{//nothing can override these special anims
			setAnimParts &= ~SETANIM_TORSO;
		}

		if ( pm->ps->legsAnimTimer
			&& PM_LockedAnim( pm->ps->legsAnim )
			&& !PM_LockedAnim( anim ) )
		{//nothing can override these special anims
			setAnimParts &= ~SETANIM_LEGS;
		}
	}

	if ( !setAnimParts )
	{
		return;
	}

	if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
	{
//		pm->ps->animationTimer = 0;

		if (setAnimParts & SETANIM_TORSO)
		{
			if( (setAnimFlags & SETANIM_FLAG_RESTART) || pm->ps->torsoAnim != anim )
			{
				PM_SetTorsoAnimTimer( pm->gent, &pm->ps->torsoAnimTimer, 0 );
			}
		}
		if (setAnimParts & SETANIM_LEGS)
		{
			if( (setAnimFlags & SETANIM_FLAG_RESTART) || pm->ps->legsAnim != anim )
			{
				PM_SetLegsAnimTimer( pm->gent, &pm->ps->legsAnimTimer, 0 );
			}
		}
	}

	PM_SetAnimFinal(&pm->ps->torsoAnim,&pm->ps->legsAnim,setAnimParts,anim,setAnimFlags,&pm->ps->torsoAnimTimer,&pm->ps->legsAnimTimer,&g_entities[pm->ps->clientNum],blendTime);//was pm->gent
}

bool TorsoAgainstWindTest( gentity_t* ent )
{
	if (ent&&//valid ent
		ent->client&&//a client
		(ent->client->ps.weapon!=WP_SABER||ent->client->ps.saberMove==LS_READY)&&//either not holding a saber or the saber is in the ready pose
		(ent->s.number<MAX_CLIENTS||G_ControlledByPlayer(ent)) &&
		gi.WE_GetWindGusting(ent->currentOrigin) &&
		gi.WE_IsOutside(ent->currentOrigin) )
	{
		if (Q_stricmp(level.mapname, "t2_wedge")!=0)
		{
			vec3_t	fwd;
			vec3_t	windDir;
			if (gi.WE_GetWindVector(windDir, ent->currentOrigin))
			{
				VectorScale(windDir, -1.0f, windDir);
				AngleVectors(pm->gent->currentAngles, fwd, 0, 0);
				if (DotProduct(fwd, windDir)>0.65f)
				{
					if (ent->client && ent->client->ps.torsoAnim!=BOTH_WIND)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_WIND, SETANIM_FLAG_NORMAL, 400);
					}
					return true;
				}
			}
		}
	}
	return false;
}

/*
-------------------------
PM_TorsoAnimLightsaber
-------------------------
*/


// Note that this function is intended to set the animation for the player, but
// only does idle-ish anims.  Anything that has a timer associated, such as attacks and blocks,
// are set by PM_WeaponLightsaber()

extern Vehicle_t *G_IsRidingVehicle( gentity_t *pEnt );
extern qboolean PM_LandingAnim( int anim );
extern qboolean PM_JumpingAnim( int anim );
qboolean PM_InCartwheel( int anim );
void PM_TorsoAnimLightsaber()
{
	// *********************************************************
	// WEAPON_READY
	// *********************************************************
	if ( pm->ps->forcePowersActive&(1<<FP_GRIP) && pm->ps->forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//holding an enemy aloft with force-grip
		return;
	}

	if ( pm->ps->forcePowersActive&(1<<FP_LIGHTNING) && pm->ps->forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
	{//lightning
		return;
	}

	if ( pm->ps->forcePowersActive&(1<<FP_DRAIN) )
	{//drain
		return;
	}

	if ( pm->ps->saber[0].blade[0].active
		&& pm->ps->saber[0].blade[0].length < 3
		&& !(pm->ps->saberEventFlags&SEF_HITWALL)
		&& pm->ps->weaponstate == WEAPON_RAISING )
	{
		if (!G_IsRidingVehicle(pm->gent))
		{
			PM_SetSaberMove(LS_DRAW);
		}
		return;
	}
	else if ( !pm->ps->SaberActive() && pm->ps->SaberLength() )
	{
		if (!G_IsRidingVehicle(pm->gent))
		{
			PM_SetSaberMove(LS_PUTAWAY);
		}
		return;
	}

	if (pm->ps->weaponTime > 0)
	{	// weapon is already busy.
		if ( pm->ps->torsoAnim == BOTH_TOSS1
			|| pm->ps->torsoAnim == BOTH_TOSS2 )
		{//in toss
			if ( !pm->ps->torsoAnimTimer )
			{//weird, get out of it, I guess
				PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
			}
		}
		return;
	}

	if (	pm->ps->weaponstate == WEAPON_READY ||
			pm->ps->weaponstate == WEAPON_CHARGING ||
			pm->ps->weaponstate == WEAPON_CHARGING_ALT )
	{//ready
		if ( pm->ps->weapon == WP_SABER && (pm->ps->SaberLength()) )
		{//saber is on
			// Select the proper idle Lightsaber attack move from the chart.
			if (pm->ps->saberMove > LS_READY && pm->ps->saberMove < LS_MOVE_MAX)
			{
				PM_SetSaberMove(saberMoveData[pm->ps->saberMove].chain_idle);
			}
			else
			{
				if ( PM_JumpingAnim( pm->ps->legsAnim )
					|| PM_LandingAnim( pm->ps->legsAnim )
					|| PM_InCartwheel( pm->ps->legsAnim )
					|| PM_FlippingAnim( pm->ps->legsAnim ))
				{
					PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
				}
				else
				{
					if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
					{//using something
						if ( !pm->ps->useTime )
						{//stopped holding it, release
							PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}//else still holding, leave it as it is
					}
					else
					{
						if ( (PM_RunningAnim( pm->ps->legsAnim )
								|| pm->ps->legsAnim == BOTH_WALK_STAFF
								|| pm->ps->legsAnim == BOTH_WALK_DUAL
								|| pm->ps->legsAnim == BOTH_WALKBACK_STAFF
								|| pm->ps->legsAnim == BOTH_WALKBACK_DUAL )
							&& pm->ps->saberBlockingTime < cg.time )
						{//running w/1-handed weapon uses full-body anim
							PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetSaberMove(LS_READY);
						}
					}
				}
			}
			/*
			if ( PM_JumpingAnim( pm->ps->legsAnim )
				|| PM_LandingAnim( pm->ps->legsAnim )
				|| PM_InCartwheel( pm->ps->legsAnim )
				|| PM_FlippingAnim( pm->ps->legsAnim ))
			{//jumping, landing cartwheel, flipping
				PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
			}
			else
			{
				PM_SetSaberMove( LS_READY );
			}
			*/
		}
		else if (TorsoAgainstWindTest(pm->gent))
		{
		}
		else if( pm->ps->legsAnim == BOTH_RUN1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN1,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_RUN2 )//&& pm->ps->saberAnimLevel != SS_STAFF )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN2,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_RUN_STAFF )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN_STAFF,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_RUN_DUAL )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN_DUAL,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_WALK1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK1,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_WALK2 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK2,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_WALK_STAFF )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK_STAFF,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_WALK_DUAL )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK_DUAL,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_CROUCH1IDLE && pm->ps->clientNum != 0 )//player falls through
		{
			//??? Why nothing?  What if you were running???
			//PM_SetAnim(pm,SETANIM_TORSO,BOTH_CROUCH1IDLE,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_JUMP1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_JUMP1,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else
		{//Used to default to both_stand1 which is an arms-down anim
//				PM_SetAnim(pm,SETANIM_TORSO,BOTH_ATTACK1,SETANIM_FLAG_NORMAL);//TORSO_WEAPONREADY1
			// Select the next proper pose for the lightsaber assuming that there are no attacks.
			if (pm->ps->saberMove > LS_READY && pm->ps->saberMove < LS_MOVE_MAX)
			{
				PM_SetSaberMove(saberMoveData[pm->ps->saberMove].chain_idle);
			}
			else
			{
				if ( PM_JumpingAnim( pm->ps->legsAnim )
					|| PM_LandingAnim( pm->ps->legsAnim )
					|| PM_InCartwheel( pm->ps->legsAnim )
					|| PM_FlippingAnim( pm->ps->legsAnim ))
				{
					PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
				}
				else
				{
					if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
					{//using something
						if ( !pm->ps->useTime )
						{//stopped holding it, release
							PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}//else still holding, leave it as it is
					}
					else
					{
						PM_SetSaberMove(LS_READY);
					}
				}
			}
		}
	}

	// *********************************************************
	// WEAPON_IDLE
	// *********************************************************

	else if ( pm->ps->weaponstate == WEAPON_IDLE )
	{
		if (TorsoAgainstWindTest(pm->gent))
		{
		}
		else if( pm->ps->legsAnim == BOTH_GUARD_LOOKAROUND1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUARD_LOOKAROUND1,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_GUARD_IDLE1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUARD_IDLE1,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_STAND1IDLE1
			|| pm->ps->legsAnim == BOTH_STAND2IDLE1
			|| pm->ps->legsAnim == BOTH_STAND2IDLE2
			|| pm->ps->legsAnim == BOTH_STAND3IDLE1
			|| pm->ps->legsAnim == BOTH_STAND5IDLE1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_STAND2TO4 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND2TO4,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_STAND4TO2 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND4TO2,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_STAND4 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND4,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else
		{
// This is now set in SetSaberMove.
			// Idle for Lightsaber
			if ( pm->gent && pm->gent->client )
			{
//				pm->gent->client->saberTrail.inAction = qfalse;
			}

			qboolean saberInAir = qtrue;
			if ( pm->ps->saberInFlight )
			{//guiding saber
				if ( PM_SaberInBrokenParry( pm->ps->saberMove ) || pm->ps->saberBlocked == BLOCKED_PARRY_BROKEN || PM_DodgeAnim( pm->ps->torsoAnim ) )
				{//we're stuck in a broken parry
					saberInAir = qfalse;
				}
				if ( pm->ps->saberEntityNum < ENTITYNUM_NONE && pm->ps->saberEntityNum > 0 )//player is 0
				{//
					if ( &g_entities[pm->ps->saberEntityNum] != NULL && g_entities[pm->ps->saberEntityNum].s.pos.trType == TR_STATIONARY )
					{//fell to the ground and we're not trying to pull it back
						saberInAir = qfalse;
					}
				}
			}
			if ( pm->ps->saberInFlight
				&& saberInAir
				&& (!pm->ps->dualSabers || !pm->ps->saber[1].Active()))
			{
				if ( !PM_ForceAnim( pm->ps->torsoAnim )
					|| pm->ps->torsoAnimTimer < 300 )
				{//don't interrupt a force power anim
					if ( pm->ps->torsoAnim != BOTH_LOSE_SABER
						|| !pm->ps->torsoAnimTimer )
					{
						PM_SetAnim( pm, SETANIM_TORSO,BOTH_SABERPULL,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
				}
			}
			else
			{//saber is on
				// Idle for Lightsaber
				if ( pm->gent && pm->gent->client )
				{
					if ( !G_InCinematicSaberAnim( pm->gent ) )
					{
						pm->gent->client->ps.SaberDeactivateTrail( 0 );
					}
				}
				// Idle for idle/ready Lightsaber
//				PM_SetAnim(pm,SETANIM_TORSO,BOTH_ATTACK1,SETANIM_FLAG_NORMAL);//TORSO_WEAPONIDLE1
				// Select the proper idle Lightsaber attack move from the chart.
				if (pm->ps->saberMove > LS_READY && pm->ps->saberMove < LS_MOVE_MAX)
				{
					PM_SetSaberMove(saberMoveData[pm->ps->saberMove].chain_idle);
				}
				else
				{
					if ( PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_LandingAnim( pm->ps->legsAnim )
						|| PM_InCartwheel( pm->ps->legsAnim )
						|| PM_FlippingAnim( pm->ps->legsAnim ))
					{
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
						{//using something
							if ( !pm->ps->useTime )
							{//stopped holding it, release
								PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							}//else still holding, leave it as it is
						}
						else
						{
							if ( (PM_RunningAnim( pm->ps->legsAnim )
								|| pm->ps->legsAnim == BOTH_WALK_STAFF
								|| pm->ps->legsAnim == BOTH_WALK_DUAL
								|| pm->ps->legsAnim == BOTH_WALKBACK_STAFF
								|| pm->ps->legsAnim == BOTH_WALKBACK_DUAL )
								&& pm->ps->saberBlockingTime < cg.time )
							{//running w/1-handed weapon uses full-body anim
								int setFlags = SETANIM_FLAG_NORMAL;
								if ( PM_LandingAnim( pm->ps->torsoAnim ) )
								{
									setFlags = SETANIM_FLAG_OVERRIDE;
								}
								PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,setFlags);
							}
							else
							{
								PM_SetSaberMove(LS_READY);
							}
						}
					}
				}
			}
		}
	}
}




/*
-------------------------
PM_TorsoAnimation
-------------------------
*/

void PM_TorsoAnimation( void )
{//FIXME: Write a much smarter and more appropriate anim picking routine logic...
//	int	oldAnim;
	if ( PM_InKnockDown( pm->ps ) || PM_InRoll( pm->ps ))
	{//in knockdown
		return;
	}

	if ( (pm->ps->eFlags&EF_HELD_BY_WAMPA) )
	{
		return;
	}

	if ( (pm->ps->eFlags&EF_FORCE_DRAINED) )
	{//being drained
		//PM_SetAnim( pm, SETANIM_TORSO, BOTH_HUGGEE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		return;
	}
	if ( (pm->ps->forcePowersActive&(1<<FP_DRAIN))
		&& pm->ps->forceDrainEntityNum < ENTITYNUM_WORLD )
	{//draining
		//PM_SetAnim( pm, SETANIM_TORSO, BOTH_HUGGER1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		return;
	}

	if( pm->gent && pm->gent->NPC && (pm->gent->NPC->scriptFlags & SCF_FORCED_MARCH) )
	{
		return;
	}

	if(pm->gent != NULL && pm->gent->client)
	{
		pm->gent->client->renderInfo.torsoFpsMod = 1.0f;
	}

	if ( pm->gent && pm->ps && pm->ps->eFlags & EF_LOCKED_TO_WEAPON )
	{
		if ( pm->gent->owner && pm->gent->owner->e_UseFunc == useF_emplaced_gun_use )//ugly way to tell, but...
		{//full body
			PM_SetAnim(pm,SETANIM_BOTH,BOTH_GUNSIT1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
		}
		else
		{//torso
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUNSIT1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
		}
		return;
	}
/*	else if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_VEHICLE && pm->ps->clientNum < MAX_CLIENTS && (m_pVehicleInfo[((CVehicleNPC *)pm->gent->NPC)->m_iVehicleTypeID].numHands == 2 || g_speederControlScheme->value == 2) )
	{//can't look around
		PM_SetAnim(pm,SETANIM_TORSO,m_pVehicleInfo[((CVehicleNPC *)pm->gent->NPC)->m_iVehicleTypeID].riderAnim,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		return;
	}*/

	if ( pm->ps->taunting > level.time )
	{
		if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_ALORA )
		{
			PM_SetAnim(pm,SETANIM_BOTH,BOTH_ALORA_TAUNT,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
		}
		else if ( pm->ps->weapon == WP_SABER && pm->ps->saberAnimLevel == SS_DUAL && PM_HasAnimation( pm->gent, BOTH_DUAL_TAUNT ) )
		{
			PM_SetAnim(pm,SETANIM_BOTH,BOTH_DUAL_TAUNT,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
		}
		else if ( pm->ps->weapon == WP_SABER
			&& pm->ps->saberAnimLevel == SS_STAFF )//pm->ps->saber[0].type == SABER_STAFF )
		{//turn on the blades
			if ( PM_HasAnimation( pm->gent, BOTH_STAFF_TAUNT ) )
			{
				PM_SetAnim(pm,SETANIM_BOTH,BOTH_STAFF_TAUNT,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
			}
			/*
			else
			{
				if ( !pm->ps->saber[0].blade[0].active )
				{//first blade is off
					//turn it on
					pm->ps->SaberBladeActivate( 0, 0, qtrue );
					if ( !pm->ps->saber[0].blade[1].active )
					{//second blade is also off, extend time of this taunt so we have enough time to turn them both on
						pm->ps->taunting = level.time + 3000;
					}
				}
				else if ( (pm->ps->taunting - level.time) < 1500 )
				{//only 1500ms left in taunt
					if ( !pm->ps->saber[0].blade[1].active )
					{//second blade is off
						//turn it on
						pm->ps->SaberBladeActivate( 0, 1, qtrue );
					}
				}
				//pose
				PM_SetAnim(pm,SETANIM_BOTH,BOTH_SABERSTAFF_STANCE,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
				pm->ps->torsoAnimTimer = pm->ps->legsAnimTimer = (pm->ps->taunting - level.time);
			}
			*/
		}
		else if ( PM_HasAnimation( pm->gent, BOTH_GESTURE1 ) )
		{
			PM_SetAnim(pm,SETANIM_BOTH,BOTH_GESTURE1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
			pm->gent->client->ps.SaberActivateTrail( 100 );
			//FIXME: will this reset?
			//FIXME: force-control (yellow glow) effect on hand and saber?
		}
		else
		{
			//PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE1,SETANIM_FLAG_NORMAL);
		}
		return;
	}

	if (pm->ps->weapon == WP_SABER )		// WP_LIGHTSABER
	{
		qboolean saberInAir = qfalse;
		if ( pm->ps->SaberLength() && !pm->ps->saberInFlight )
		{
			PM_TorsoAnimLightsaber();
		}
		else
		{
			if ( pm->ps->forcePowersActive&(1<<FP_GRIP) && pm->ps->forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
			{//holding an enemy aloft with force-grip
				return;
			}
			if ( pm->ps->forcePowersActive&(1<<FP_LIGHTNING) && pm->ps->forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
			{//lightning
				return;
			}
			if ( pm->ps->forcePowersActive&(1<<FP_DRAIN) )
			{//drain
				return;
			}

			saberInAir = qtrue;

			if ( PM_SaberInBrokenParry( pm->ps->saberMove ) || pm->ps->saberBlocked == BLOCKED_PARRY_BROKEN || PM_DodgeAnim( pm->ps->torsoAnim ) )
			{//we're stuck in a broken parry
				PM_TorsoAnimLightsaber();
			}
			else
			{
				if ( pm->ps->saberEntityNum < ENTITYNUM_NONE && pm->ps->saberEntityNum > 0 )//player is 0
					{//
					if ( &g_entities[pm->ps->saberEntityNum] != NULL && g_entities[pm->ps->saberEntityNum].s.pos.trType == TR_STATIONARY )
					{//fell to the ground and we're not trying to pull it back
						saberInAir = qfalse;
					}
				}

				if ( pm->ps->saberInFlight
					&& saberInAir
					&& (!pm->ps->dualSabers //not using 2 sabers
						|| !pm->ps->saber[1].Active() //left one off
						|| pm->ps->torsoAnim == BOTH_SABERDUAL_STANCE//not attacking
						|| pm->ps->torsoAnim == BOTH_SABERPULL//not attacking
						|| pm->ps->torsoAnim == BOTH_STAND1//not attacking
						|| PM_RunningAnim( pm->ps->torsoAnim ) //not attacking
						|| PM_WalkingAnim( pm->ps->torsoAnim ) //not attacking
						|| PM_JumpingAnim( pm->ps->torsoAnim )//not attacking
						|| PM_SwimmingAnim( pm->ps->torsoAnim ) )//not attacking
					)
				{
					if ( !PM_ForceAnim( pm->ps->torsoAnim ) || pm->ps->torsoAnimTimer < 300 )
					{//don't interrupt a force power anim
						if ( pm->ps->torsoAnim != BOTH_LOSE_SABER
							|| !pm->ps->torsoAnimTimer )
						{
							PM_SetAnim( pm, SETANIM_TORSO,BOTH_SABERPULL,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}
					}
				}
				else
				{
					if ( PM_InSlopeAnim( pm->ps->legsAnim ) )
					{//HMM... this probably breaks the saber putaway and select anims
						if ( pm->ps->SaberLength() > 0 )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND2,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
						}
					}
					else
					{
						if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
						{//using something
							if ( !pm->ps->useTime )
							{//stopped holding it, release
								PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							}//else still holding, leave it as it is
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
						}
					}
				}
			}
		}

		if (pm->ps->weaponTime<= 0 && (pm->ps->saberMove==LS_READY || pm->ps->SaberLength()==0) && !saberInAir)
		{
			TorsoAgainstWindTest(pm->gent);
		}
		return;
	}

	if ( PM_ForceAnim( pm->ps->torsoAnim )
		&& pm->ps->torsoAnimTimer > 0 )
	{//in a force anim, don't do a stand anim
		return;
	}


	qboolean weaponBusy = qfalse;

	if ( pm->ps->weapon == WP_NONE )
	{
		weaponBusy = qfalse;
	}
	else if ( pm->ps->weaponstate == WEAPON_FIRING || pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_CHARGING_ALT )
	{
		weaponBusy = qtrue;
	}
	else if ( pm->ps->lastShotTime > level.time - 3000 )
	{
		weaponBusy = qtrue;
	}
	else if ( pm->ps->weaponTime > 0 )
	{
		weaponBusy = qtrue;
	}
	else if ( pm->gent && pm->gent->client->fireDelay > 0 )
	{
		weaponBusy = qtrue;
	}
	else if ( TorsoAgainstWindTest(pm->gent) )
	{
		return;
	}
	else if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && cg.zoomTime > cg.time - 5000 )
	{//if we used binoculars recently, aim weapon
		weaponBusy = qtrue;
		pm->ps->weaponstate = WEAPON_IDLE;
	}
	else if ( pm->ps->pm_flags & PMF_DUCKED )
	{//ducking is considered on alert... plus looks stupid to have arms hanging down when crouched
		weaponBusy = qtrue;
	}

	if (	pm->ps->weapon == WP_NONE ||
			pm->ps->weaponstate == WEAPON_READY ||
			pm->ps->weaponstate == WEAPON_CHARGING ||
			pm->ps->weaponstate == WEAPON_CHARGING_ALT )
	{
		if ( pm->ps->weapon == WP_SABER && pm->ps->SaberLength() )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_ATTACK1,SETANIM_FLAG_NORMAL);//TORSO_WEAPONREADY1
		}
		else if( pm->ps->legsAnim == BOTH_RUN1 && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_RUN2 && !weaponBusy )//&& pm->ps->saberAnimLevel != SS_STAFF )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN2,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_RUN4 && !weaponBusy )//&& pm->ps->saberAnimLevel != SS_STAFF )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN4,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_RUN_STAFF && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN_STAFF,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_RUN_DUAL && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN_DUAL,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_WALK1 && !weaponBusy  )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_WALK2 && !weaponBusy  )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK2,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_WALK_STAFF && !weaponBusy  )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK_STAFF,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_WALK_DUAL&& !weaponBusy  )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK_DUAL,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_CROUCH1IDLE && pm->ps->clientNum != 0 )//player falls through
		{
			//??? Why nothing?  What if you were running???
			//PM_SetAnim(pm,SETANIM_TORSO,BOTH_CROUCH1IDLE,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_JUMP1 && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_JUMP1,SETANIM_FLAG_NORMAL, 100);	// Only blend over 100ms
		}
		else if( pm->ps->legsAnim == BOTH_SWIM_IDLE1 && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_SWIM_IDLE1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_SWIMFORWARD && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_SWIMFORWARD,SETANIM_FLAG_NORMAL);
		}
		else if ( pm->ps->weapon == WP_NONE )
		{
			int legsAnim = pm->ps->legsAnim;
			/*
			if ( PM_RollingAnim( legsAnim ) ||
				PM_FlippingAnim( legsAnim ) ||
				PM_JumpingAnim( legsAnim ) ||
				PM_PainAnim( legsAnim ) ||
				PM_SwimmingAnim( legsAnim ) )
			*/
			{
				PM_SetAnim(pm, SETANIM_TORSO, legsAnim, SETANIM_FLAG_NORMAL );
			}
		}
		else
		{//Used to default to both_stand1 which is an arms-down anim
			if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
			{//using something
				if ( !pm->ps->useTime )
				{//stopped holding it, release
					PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}//else still holding, leave it as it is
			}
			else if ( pm->gent != NULL
				&& (pm->gent->s.number<MAX_CLIENTS||G_ControlledByPlayer(pm->gent))
				&& pm->ps->weaponstate != WEAPON_CHARGING
				&& pm->ps->weaponstate != WEAPON_CHARGING_ALT )
			{//PLayer- temp hack for weapon frame
				if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_RANCOR )
				{//ignore
				}
				else if ( pm->ps->weapon == WP_MELEE )
				{//hehe
					PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND6,SETANIM_FLAG_NORMAL);
				}
				else
				{
					PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
				}
			}
			else if ( PM_InSpecialJump( pm->ps->legsAnim ) )
			{//use legs anim
				//FIXME: or just use whatever's currently playing?
				//PM_SetAnim( pm, SETANIM_TORSO, pm->ps->legsAnim, SETANIM_FLAG_NORMAL );
			}
			else
			{
				switch(pm->ps->weapon)
				{
				// ********************************************************
				case WP_SABER:		// WP_LIGHTSABER
					// Ready pose for Lightsaber
//					PM_SetAnim(pm,SETANIM_TORSO,BOTH_ATTACK1,SETANIM_FLAG_NORMAL);//TORSO_WEAPONREADY1
					// Select the next proper pose for the lightsaber assuming that there are no attacks.
					if (pm->ps->saberMove > LS_NONE && pm->ps->saberMove < LS_MOVE_MAX)
					{
						PM_SetSaberMove(saberMoveData[pm->ps->saberMove].chain_idle);
					}
					break;
				// ********************************************************

				case WP_BRYAR_PISTOL:
					//FIXME: if recently fired, hold the ready!
					if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT || weaponBusy )
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY2,SETANIM_FLAG_NORMAL);
					}
					else if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY2,SETANIM_FLAG_NORMAL);
					}
					break;
				case WP_BLASTER_PISTOL:
					if ( pm->gent
						&& pm->gent->weaponModel[1] > 0 )
					{//dual pistols
						if ( weaponBusy )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUNSIT1,SETANIM_FLAG_NORMAL);
						}
						else if ( PM_RunningAnim( pm->ps->legsAnim )
							|| PM_WalkingAnim( pm->ps->legsAnim )
							|| PM_JumpingAnim( pm->ps->legsAnim )
							|| PM_SwimmingAnim( pm->ps->legsAnim ) )
						{//running w/1-handed weapon uses full-body anim
							PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND6,SETANIM_FLAG_NORMAL);
						}
					}
					else
					{//single pistols
						if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT || weaponBusy )
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY2,SETANIM_FLAG_NORMAL);
						}
						else if ( PM_RunningAnim( pm->ps->legsAnim )
							|| PM_WalkingAnim( pm->ps->legsAnim )
							|| PM_JumpingAnim( pm->ps->legsAnim )
							|| PM_SwimmingAnim( pm->ps->legsAnim ) )
						{//running w/1-handed weapon uses full-body anim
							PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY2,SETANIM_FLAG_NORMAL);
						}
					}
					break;
				case WP_NONE:
					//NOTE: should never get here
					break;
				case WP_MELEE:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_RANCOR )
						{//ignore
						}
						else if ( pm->gent && pm->gent->client && !PM_DroidMelee( pm->gent->client->NPC_class ) )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND6,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
						}
					}
					break;
				case WP_TUSKEN_STAFF:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm, SETANIM_TORSO, BOTH_STAND3, SETANIM_FLAG_NORMAL);
					}
					break;

				case WP_NOGHRI_STICK:
					PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_ATTACK2,SETANIM_FLAG_NORMAL);
					break;

				case WP_BLASTER:
					PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_ATTACK2,SETANIM_FLAG_NORMAL);
					break;
				case WP_DISRUPTOR:
				case WP_TUSKEN_RIFLE:
					if ( (pm->ps->weaponstate != WEAPON_FIRING
							&& pm->ps->weaponstate != WEAPON_CHARGING
							&& pm->ps->weaponstate != WEAPON_CHARGING_ALT)
							|| PM_RunningAnim( pm->ps->legsAnim )
							|| PM_WalkingAnim( pm->ps->legsAnim )
							|| PM_JumpingAnim( pm->ps->legsAnim )
							|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running sniper weapon uses normal ready
						if ( pm->ps->clientNum )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
						}
					}
					else
					{
						if ( pm->ps->clientNum )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );//TORSO_WEAPONREADY4//SETANIM_FLAG_RESTART|
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY4, SETANIM_FLAG_NORMAL );
						}
					}
					break;
				case WP_BOT_LASER:
					PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE2,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD);
					break;
				case WP_THERMAL:
					if ( pm->ps->weaponstate != WEAPON_FIRING
						&& pm->ps->weaponstate != WEAPON_CHARGING
						&& pm->ps->weaponstate != WEAPON_CHARGING_ALT
						&& (PM_RunningAnim( pm->ps->legsAnim )
							|| PM_WalkingAnim( pm->ps->legsAnim )
							|| PM_JumpingAnim( pm->ps->legsAnim )
							|| PM_SwimmingAnim( pm->ps->legsAnim )) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_CHARGING_ALT) )
						{//player pulling back to throw
							if ( PM_StandingAnim( pm->ps->legsAnim ) )
							{
								PM_SetAnim( pm, SETANIM_LEGS, BOTH_THERMAL_READY, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							}
							else if ( pm->ps->legsAnim == BOTH_THERMAL_READY )
							{//sigh... hold it so pm_footsteps doesn't override
								if ( pm->ps->legsAnimTimer < 100 )
								{
									pm->ps->legsAnimTimer = 100;
								}
							}
							PM_SetAnim( pm, SETANIM_TORSO, BOTH_THERMAL_READY, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						}
						else
						{
							if ( weaponBusy )
							{
								PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY10, SETANIM_FLAG_NORMAL );
							}
							else
							{
								PM_SetAnim( pm, SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_NORMAL );
							}
						}
					}
					break;
				case WP_REPEATER:
					if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_GALAKMECH )
					{//
						if ( pm->gent->alt_fire )
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY1,SETANIM_FLAG_NORMAL);
						}
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					}
					break;
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( weaponBusy )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_NORMAL );
						}
					}
					break;
				default:
					PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					break;
				}
			}
		}
	}
	else if ( pm->ps->weaponstate == WEAPON_IDLE )
	{
		if( pm->ps->legsAnim == BOTH_GUARD_LOOKAROUND1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUARD_LOOKAROUND1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_GUARD_IDLE1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUARD_IDLE1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_STAND1IDLE1
			|| pm->ps->legsAnim == BOTH_STAND2IDLE1
			|| pm->ps->legsAnim == BOTH_STAND2IDLE2
			|| pm->ps->legsAnim == BOTH_STAND3IDLE1
			|| pm->ps->legsAnim == BOTH_STAND5IDLE1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_STAND2TO4 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND2TO4,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_STAND4TO2 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND4TO2,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_STAND4 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND4,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_SWIM_IDLE1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_SWIM_IDLE1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_SWIMFORWARD )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_SWIMFORWARD,SETANIM_FLAG_NORMAL);
		}
		else if ( PM_InSpecialJump( pm->ps->legsAnim ) )
		{//use legs anim
			//FIXME: or just use whatever's currently playing?
			//PM_SetAnim( pm, SETANIM_TORSO, pm->ps->legsAnim, SETANIM_FLAG_NORMAL );
		}
		else if ( (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()) && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
		{//using something
			if ( !pm->ps->useTime )
			{//stopped holding it, release
				PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}//else still holding, leave it as it is
		}
		else
		{
			if ( !weaponBusy
				&& pm->ps->weapon != WP_BOWCASTER
				&& pm->ps->weapon != WP_REPEATER
				&& pm->ps->weapon != WP_FLECHETTE
				&& pm->ps->weapon != WP_ROCKET_LAUNCHER
				&& pm->ps->weapon != WP_CONCUSSION
				&& ( PM_RunningAnim( pm->ps->legsAnim )
					|| (PM_WalkingAnim( pm->ps->legsAnim ) && (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer()))
					|| PM_JumpingAnim( pm->ps->legsAnim )
					|| PM_SwimmingAnim( pm->ps->legsAnim ) ) )
			{//running w/1-handed or light 2-handed weapon uses full-body anim if you're not using the weapon right now
				PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
			}
			else
			{
				switch ( pm->ps->weapon )
				{
				// ********************************************************
				case WP_SABER:		// WP_LIGHTSABER
					// Shouldn't get here, should go to TorsoAnimLightsaber
					break;
				// ********************************************************

				case WP_BRYAR_PISTOL:
					if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT || weaponBusy )
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY2,SETANIM_FLAG_NORMAL);
					}
					else if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE2,SETANIM_FLAG_NORMAL);
					}
					break;
				case WP_BLASTER_PISTOL:
					if ( pm->gent
						&& pm->gent->weaponModel[1] > 0 )
					{//dual pistols
						if ( weaponBusy )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_GUNSIT1,SETANIM_FLAG_NORMAL);
						}
						else if ( PM_RunningAnim( pm->ps->legsAnim )
							|| PM_WalkingAnim( pm->ps->legsAnim )
							|| PM_JumpingAnim( pm->ps->legsAnim )
							|| PM_SwimmingAnim( pm->ps->legsAnim ) )
						{//running w/1-handed weapon uses full-body anim
							PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
						}
					}
					else
					{//single pistols
						if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT || weaponBusy )
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY2,SETANIM_FLAG_NORMAL);
						}
						else if ( PM_RunningAnim( pm->ps->legsAnim )
								|| PM_WalkingAnim( pm->ps->legsAnim )
								|| PM_JumpingAnim( pm->ps->legsAnim )
								|| PM_SwimmingAnim( pm->ps->legsAnim ) )
						{//running w/1-handed weapon uses full-body anim
							PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE2,SETANIM_FLAG_NORMAL);
						}
					}
					break;

				case WP_NONE:
					//NOTE: should never get here
					break;

				case WP_MELEE:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_RANCOR )
						{//ignore
						}
						else if ( pm->gent && pm->gent->client && !PM_DroidMelee( pm->gent->client->NPC_class ) )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND6,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
						}
					}
					break;

				case WP_TUSKEN_STAFF:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm, SETANIM_TORSO, BOTH_STAND3, SETANIM_FLAG_NORMAL);
					}
					break;

				case WP_NOGHRI_STICK:
					if ( weaponBusy )
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					}
					break;

				case WP_BLASTER:
					if ( weaponBusy )
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					}
					break;

				case WP_DISRUPTOR:
				case WP_TUSKEN_RIFLE:
					if ( (pm->ps->weaponstate != WEAPON_FIRING
							&& pm->ps->weaponstate != WEAPON_CHARGING
							&& pm->ps->weaponstate != WEAPON_CHARGING_ALT)
							|| PM_RunningAnim( pm->ps->legsAnim )
							|| PM_WalkingAnim( pm->ps->legsAnim )
							|| PM_JumpingAnim( pm->ps->legsAnim )
							|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running sniper weapon uses normal ready
						if ( pm->ps->clientNum )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
						}
					}
					else
					{
						if ( pm->ps->clientNum )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY4, SETANIM_FLAG_NORMAL );
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY4, SETANIM_FLAG_NORMAL );
						}
					}
					break;

				case WP_BOT_LASER:
					PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE2,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD);
					break;

				case WP_THERMAL:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( weaponBusy )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONIDLE10, SETANIM_FLAG_NORMAL );
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_NORMAL );
						}
					}
					break;

				case WP_REPEATER:
					if ( weaponBusy )
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					}
					break;
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					if ( PM_RunningAnim( pm->ps->legsAnim )
						|| PM_WalkingAnim( pm->ps->legsAnim )
						|| PM_JumpingAnim( pm->ps->legsAnim )
						|| PM_SwimmingAnim( pm->ps->legsAnim ) )
					{//running w/1-handed weapon uses full-body anim
						PM_SetAnim(pm,SETANIM_TORSO,pm->ps->legsAnim,SETANIM_FLAG_NORMAL);
					}
					else
					{
						if ( weaponBusy )
						{
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONIDLE3, SETANIM_FLAG_NORMAL );
						}
						else
						{
							PM_SetAnim( pm, SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_NORMAL );
						}
					}
					break;

				default:
					if ( weaponBusy )
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					}
					break;
				}
			}
		}
	}
}

//=========================================================================
// Anim checking utils
//=========================================================================

int PM_GetTurnAnim( gentity_t *gent, int anim )
{
	if ( !gent )
	{
		return -1;
	}

	switch( anim )
	{
	case BOTH_STAND1:			//# Standing idle: no weapon: hands down
	case BOTH_STAND1IDLE1:		//# Random standing idle
	case BOTH_STAND2:			//# Standing idle with a weapon
	case BOTH_SABERFAST_STANCE:
	case BOTH_SABERSLOW_STANCE:
	case BOTH_STAND2IDLE1:		//# Random standing idle
	case BOTH_STAND2IDLE2:		//# Random standing idle
	case BOTH_STAND3:			//# Standing hands behind back: at ease: etc.
	case BOTH_STAND3IDLE1:		//# Random standing idle
	case BOTH_STAND4:			//# two handed: gun down: relaxed stand
	case BOTH_STAND5:			//# standing idle, no weapon, hand down, back straight
	case BOTH_STAND5IDLE1:		//# Random standing idle
	case BOTH_STAND6:			//# one handed: gun at side: relaxed stand
	case BOTH_STAND2TO4:			//# Transition from stand2 to stand4
	case BOTH_STAND4TO2:			//# Transition from stand4 to stand2
	case BOTH_GESTURE1:			//# Generic gesture: non-specific
	case BOTH_GESTURE2:			//# Generic gesture: non-specific
	case BOTH_TALK1:				//# Generic talk anim
	case BOTH_TALK2:				//# Generic talk anim
		if ( PM_HasAnimation( gent, LEGS_TURN1 ) )
		{
			return LEGS_TURN1;
		}
		else
		{
			return -1;
		}
		break;
	case BOTH_ATTACK1:			//# Attack with generic 1-handed weapon
	case BOTH_ATTACK2:			//# Attack with generic 2-handed weapon
	case BOTH_ATTACK3:			//# Attack with heavy 2-handed weapon
	case BOTH_ATTACK4:			//# Attack with ???
	case BOTH_MELEE1:			//# First melee attack
	case BOTH_MELEE2:			//# Second melee attack
	case BOTH_GUARD_LOOKAROUND1:	//# Cradling weapon and looking around
	case BOTH_GUARD_IDLE1:		//# Cradling weapon and standing
		if ( PM_HasAnimation( gent, LEGS_TURN2 ) )
		{
			return LEGS_TURN2;
		}
		else
		{
			return -1;
		}
		break;
	default:
		return -1;
		break;
	}
}

int PM_TurnAnimForLegsAnim( gentity_t *gent, int anim )
{
	if ( !gent )
	{
		return -1;
	}

	switch( anim )
	{
	case BOTH_STAND1:			//# Standing idle: no weapon: hands down
	case BOTH_STAND1IDLE1:		//# Random standing idle
		if ( PM_HasAnimation( gent, BOTH_TURNSTAND1 ) )
		{
			return BOTH_TURNSTAND1;
		}
		else
		{
			return -1;
		}
		break;
	case BOTH_STAND2:			//# Standing idle with a weapon
	case BOTH_SABERFAST_STANCE:
	case BOTH_SABERSLOW_STANCE:
	case BOTH_STAND2IDLE1:		//# Random standing idle
	case BOTH_STAND2IDLE2:		//# Random standing idle
		if ( PM_HasAnimation( gent, BOTH_TURNSTAND2 ) )
		{
			return BOTH_TURNSTAND2;
		}
		else
		{
			return -1;
		}
		break;
	case BOTH_STAND3:			//# Standing hands behind back: at ease: etc.
	case BOTH_STAND3IDLE1:		//# Random standing idle
		if ( PM_HasAnimation( gent, BOTH_TURNSTAND3 ) )
		{
			return BOTH_TURNSTAND3;
		}
		else
		{
			return -1;
		}
		break;
	case BOTH_STAND4:			//# two handed: gun down: relaxed stand
		if ( PM_HasAnimation( gent, BOTH_TURNSTAND4 ) )
		{
			return BOTH_TURNSTAND4;
		}
		else
		{
			return -1;
		}
		break;
	case BOTH_STAND5:			//# standing idle, no weapon, hand down, back straight
	case BOTH_STAND5IDLE1:		//# Random standing idle
		if ( PM_HasAnimation( gent, BOTH_TURNSTAND5 ) )
		{
			return BOTH_TURNSTAND5;
		}
		else
		{
			return -1;
		}
		break;
	case BOTH_CROUCH1:			//# Transition from standing to crouch
	case BOTH_CROUCH1IDLE:		//# Crouching idle
	/*
	case BOTH_UNCROUCH1:			//# Transition from crouch to standing
	case BOTH_CROUCH2TOSTAND1:	//# going from crouch2 to stand1
	case BOTH_CROUCH3:			//# Desann crouching down to Kyle (cin 9)
	case BOTH_UNCROUCH3:			//# Desann uncrouching down to Kyle (cin 9)
	case BOTH_CROUCH4:			//# Slower version of crouch1 for cinematics
	case BOTH_UNCROUCH4:			//# Slower version of uncrouch1 for cinematics
	*/
		if ( PM_HasAnimation( gent, BOTH_TURNCROUCH1 ) )
		{
			return BOTH_TURNCROUCH1;
		}
		else
		{
			return -1;
		}
		break;
	default:
		return -1;
		break;
	}
}

qboolean PM_InOnGroundAnim ( playerState_t *ps )
{
	switch( ps->legsAnim )
	{
	case BOTH_DEAD1:
	case BOTH_DEAD2:
	case BOTH_DEAD3:
	case BOTH_DEAD4:
	case BOTH_DEAD5:
	case BOTH_DEADFORWARD1:
	case BOTH_DEADBACKWARD1:
	case BOTH_DEADFORWARD2:
	case BOTH_DEADBACKWARD2:
	case BOTH_LYINGDEATH1:
	case BOTH_LYINGDEAD1:
	case BOTH_SLEEP1:			//# laying on back-rknee up-rhand on torso
		return qtrue;
		break;
	case BOTH_KNOCKDOWN1:		//#
	case BOTH_KNOCKDOWN2:		//#
	case BOTH_KNOCKDOWN3:		//#
	case BOTH_KNOCKDOWN4:		//#
	case BOTH_KNOCKDOWN5:		//#
	case BOTH_LK_DL_ST_T_SB_1_L:
	case BOTH_RELEASED:
		if ( ps->legsAnimTimer < 500 )
		{//pretty much horizontal by this point
			return qtrue;
		}
		break;
	case BOTH_PLAYER_PA_3_FLY:
		if ( ps->legsAnimTimer < 300 )
		{//pretty much horizontal by this point
			return qtrue;
		}
		/*
		else if ( ps->clientNum < MAX_CLIENTS
			&& ps->legsAnimTimer < 300 + PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME )
		{
			return qtrue;
		}
		*/
		break;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
		if ( ps->legsAnimTimer > PM_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, (animNumber_t)ps->legsAnim )-400 )
		{//still pretty much horizontal at this point
			return qtrue;
		}
		break;
	}

	return qfalse;
}

qboolean PM_InSpecialDeathAnim( int anim )
{
	switch( pm->ps->legsAnim )
	{
	case BOTH_DEATH_ROLL:		//# Death anim from a roll
	case BOTH_DEATH_FLIP:		//# Death anim from a flip
	case BOTH_DEATH_SPIN_90_R:	//# Death anim when facing 90 degrees right
	case BOTH_DEATH_SPIN_90_L:	//# Death anim when facing 90 degrees left
	case BOTH_DEATH_SPIN_180:	//# Death anim when facing backwards
	case BOTH_DEATH_LYING_UP:	//# Death anim when lying on back
	case BOTH_DEATH_LYING_DN:	//# Death anim when lying on front
	case BOTH_DEATH_FALLING_DN:	//# Death anim when falling on face
	case BOTH_DEATH_FALLING_UP:	//# Death anim when falling on back
	case BOTH_DEATH_CROUCHED:	//# Death anim when crouched
		return qtrue;
		break;
	default:
		return qfalse;
		break;
	}
}

qboolean PM_InDeathAnim ( void )
{//Purposely does not cover stumbledeath and falldeath...
	switch( pm->ps->legsAnim )
	{
	case BOTH_DEATH1:		//# First Death anim
	case BOTH_DEATH2:			//# Second Death anim
	case BOTH_DEATH3:			//# Third Death anim
	case BOTH_DEATH4:			//# Fourth Death anim
	case BOTH_DEATH5:			//# Fifth Death anim
	case BOTH_DEATH6:			//# Sixth Death anim
	case BOTH_DEATH7:			//# Seventh Death anim
	case BOTH_DEATH8:			//#
	case BOTH_DEATH9:			//#
	case BOTH_DEATH10:			//#
	case BOTH_DEATH11:			//#
	case BOTH_DEATH12:			//#
	case BOTH_DEATH13:			//#
	case BOTH_DEATH14:			//#
	case BOTH_DEATH14_UNGRIP:	//# Desann's end death (cin #35)
	case BOTH_DEATH14_SITUP:		//# Tavion sitting up after having been thrown (cin #23)
	case BOTH_DEATH15:			//#
	case BOTH_DEATH16:			//#
	case BOTH_DEATH17:			//#
	case BOTH_DEATH18:			//#
	case BOTH_DEATH19:			//#
	case BOTH_DEATH20:			//#
	case BOTH_DEATH21:			//#
	case BOTH_DEATH22:			//#
	case BOTH_DEATH23:			//#
	case BOTH_DEATH24:			//#
	case BOTH_DEATH25:			//#

	case BOTH_DEATHFORWARD1:		//# First Death in which they get thrown forward
	case BOTH_DEATHFORWARD2:		//# Second Death in which they get thrown forward
	case BOTH_DEATHFORWARD3:		//# Tavion's falling in cin# 23
	case BOTH_DEATHBACKWARD1:	//# First Death in which they get thrown backward
	case BOTH_DEATHBACKWARD2:	//# Second Death in which they get thrown backward

	case BOTH_DEATH1IDLE:		//# Idle while close to death
	case BOTH_LYINGDEATH1:		//# Death to play when killed lying down
	case BOTH_STUMBLEDEATH1:		//# Stumble forward and fall face first death
	case BOTH_FALLDEATH1:		//# Fall forward off a high cliff and splat death - start
	case BOTH_FALLDEATH1INAIR:	//# Fall forward off a high cliff and splat death - loop
	case BOTH_FALLDEATH1LAND:	//# Fall forward off a high cliff and splat death - hit bottom
	//# #sep case BOTH_ DEAD POSES # Should be last frame of corresponding previous anims
	case BOTH_DEAD1:				//# First Death finished pose
	case BOTH_DEAD2:				//# Second Death finished pose
	case BOTH_DEAD3:				//# Third Death finished pose
	case BOTH_DEAD4:				//# Fourth Death finished pose
	case BOTH_DEAD5:				//# Fifth Death finished pose
	case BOTH_DEAD6:				//# Sixth Death finished pose
	case BOTH_DEAD7:				//# Seventh Death finished pose
	case BOTH_DEAD8:				//#
	case BOTH_DEAD9:				//#
	case BOTH_DEAD10:			//#
	case BOTH_DEAD11:			//#
	case BOTH_DEAD12:			//#
	case BOTH_DEAD13:			//#
	case BOTH_DEAD14:			//#
	case BOTH_DEAD15:			//#
	case BOTH_DEAD16:			//#
	case BOTH_DEAD17:			//#
	case BOTH_DEAD18:			//#
	case BOTH_DEAD19:			//#
	case BOTH_DEAD20:			//#
	case BOTH_DEAD21:			//#
	case BOTH_DEAD22:			//#
	case BOTH_DEAD23:			//#
	case BOTH_DEAD24:			//#
	case BOTH_DEAD25:			//#
	case BOTH_DEADFORWARD1:		//# First thrown forward death finished pose
	case BOTH_DEADFORWARD2:		//# Second thrown forward death finished pose
	case BOTH_DEADBACKWARD1:		//# First thrown backward death finished pose
	case BOTH_DEADBACKWARD2:		//# Second thrown backward death finished pose
	case BOTH_LYINGDEAD1:		//# Killed lying down death finished pose
	case BOTH_STUMBLEDEAD1:		//# Stumble forward death finished pose
	case BOTH_FALLDEAD1LAND:		//# Fall forward and splat death finished pose
	//# #sep case BOTH_ DEAD TWITCH/FLOP # React to being shot from death poses
	case BOTH_DEADFLOP1:		//# React to being shot from First Death finished pose
	case BOTH_DEADFLOP2:		//# React to being shot from Second Death finished pose
	case BOTH_DISMEMBER_HEAD1:	//#
	case BOTH_DISMEMBER_TORSO1:	//#
	case BOTH_DISMEMBER_LLEG:	//#
	case BOTH_DISMEMBER_RLEG:	//#
	case BOTH_DISMEMBER_RARM:	//#
	case BOTH_DISMEMBER_LARM:	//#
		return qtrue;
		break;
	default:
		return PM_InSpecialDeathAnim( pm->ps->legsAnim );
		break;
	}
}

qboolean PM_InCartwheel( int anim )
{
	switch ( anim )
	{
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_InButterfly( int anim )
{
	switch ( anim )
	{
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_StandingAnim( int anim )
{//NOTE: does not check idles or special (cinematic) stands
	switch ( anim )
	{
	case BOTH_STAND1:
	case BOTH_STAND2:
	case BOTH_STAND3:
	case BOTH_STAND4:
	case BOTH_ATTACK3:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_InAirKickingAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_KickingAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	//NOT a kick, but acts like one:
	case BOTH_A7_HILT:
	//NOT kicks, but do kick traces anyway
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
		return qtrue;
		break;
	default:
		return PM_InAirKickingAnim( anim );
		break;
	}
	//return qfalse;
}

qboolean PM_StabDownAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_GoingToAttackDown( playerState_t *ps )
{
	if ( PM_StabDownAnim( ps->torsoAnim )//stabbing downward
		|| ps->saberMove == LS_A_LUNGE//lunge
		|| ps->saberMove == LS_A_JUMP_T__B_//death from above
		|| ps->saberMove == LS_A_T2B//attacking top to bottom
		|| ps->saberMove == LS_S_T2B//starting at attack downward
		|| (PM_SaberInTransition( ps->saberMove ) && saberMoveData[ps->saberMove].endQuad == Q_T) )//transitioning to a top to bottom attack
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_ForceUsingSaberAnim( int anim )
{//saber/acrobatic anims that should prevent you from recharging force power while you're in them...
	switch ( anim )
	{
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_FORCELONGLEAP_START:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_FORCEWALLRUNFLIP_END:
	case BOTH_FORCEWALLRUNFLIP_ALT:
	case BOTH_FORCEWALLREBOUND_FORWARD:
	case BOTH_FORCEWALLREBOUND_LEFT:
	case BOTH_FORCEWALLREBOUND_BACK:
	case BOTH_FORCEWALLREBOUND_RIGHT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_FLIP_HOLD7:
	case BOTH_FLIP_LAND:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
	case BOTH_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_ALORA_FLIP_B:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FORCEJUMP1:
	case BOTH_FORCEINAIR1:
	case BOTH_FORCELAND1:
	case BOTH_FORCEJUMPBACK1:
	case BOTH_FORCEINAIRBACK1:
	case BOTH_FORCELANDBACK1:
	case BOTH_FORCEJUMPLEFT1:
	case BOTH_FORCEINAIRLEFT1:
	case BOTH_FORCELANDLEFT1:
	case BOTH_FORCEJUMPRIGHT1:
	case BOTH_FORCEINAIRRIGHT1:
	case BOTH_FORCELANDRIGHT1:
	case BOTH_FLIP_F:
	case BOTH_FLIP_B:
	case BOTH_FLIP_L:
	case BOTH_FLIP_R:
	case BOTH_ALORA_FLIP_1:
	case BOTH_ALORA_FLIP_2:
	case BOTH_ALORA_FLIP_3:
	case BOTH_DODGE_FL:
	case BOTH_DODGE_FR:
	case BOTH_DODGE_BL:
	case BOTH_DODGE_BR:
	case BOTH_DODGE_L:
	case BOTH_DODGE_R:
	case BOTH_DODGE_HOLD_FL:
	case BOTH_DODGE_HOLD_FR:
	case BOTH_DODGE_HOLD_BL:
	case BOTH_DODGE_HOLD_BR:
	case BOTH_DODGE_HOLD_L:
	case BOTH_DODGE_HOLD_R:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_WALL_FLIP_BACK2:
	case BOTH_SPIN1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_DEFLECTSLASH__R__L_FIN:
	case BOTH_ARIAL_F1:
		return qtrue;
	}
	return qfalse;
}

qboolean G_HasKnockdownAnims( gentity_t *ent )
{
	if ( PM_HasAnimation( ent, BOTH_KNOCKDOWN1 )
		&& PM_HasAnimation( ent, BOTH_KNOCKDOWN2 )
		&& PM_HasAnimation( ent, BOTH_KNOCKDOWN3 )
		&& PM_HasAnimation( ent, BOTH_KNOCKDOWN4 )
		&& PM_HasAnimation( ent, BOTH_KNOCKDOWN5 ) )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_InAttackRoll( int anim )
{
	switch ( anim )
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_LockedAnim( int anim )
{//anims that can *NEVER* be overridden, regardless
	switch ( anim )
	{
	case BOTH_KYLE_PA_1:
	case BOTH_KYLE_PA_2:
	case BOTH_KYLE_PA_3:
	case BOTH_PLAYER_PA_1:
	case BOTH_PLAYER_PA_2:
	case BOTH_PLAYER_PA_3:
	case BOTH_PLAYER_PA_3_FLY:
	case BOTH_TAVION_SCEPTERGROUND:
	case BOTH_TAVION_SWORDPOWER:
	case BOTH_SCEPTER_START:
	case BOTH_SCEPTER_HOLD:
	case BOTH_SCEPTER_STOP:
	//grabbed by wampa
	case BOTH_GRABBED:	//#
	case BOTH_RELEASED:	//#	when Wampa drops player, transitions into fall on back
	case BOTH_HANG_IDLE:	//#
	case BOTH_HANG_ATTACK:	//#
	case BOTH_HANG_PAIN:	//#
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SuperBreakLoseAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_LK_S_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_S_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_S_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_S_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_S_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_S_T_SB_1_L:	//super break I lost
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_SuperBreakWinAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_LK_S_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_S_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_S_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_S_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_S_S_S_SB_1_W:	//super break I won
	case BOTH_LK_S_S_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_S_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_S_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_S_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_S_T_SB_1_W:	//super break I won
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_SaberLockBreakAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_BF1BREAK:
	case BOTH_BF2BREAK:
	case BOTH_CWCIRCLEBREAK:
	case BOTH_CCWCIRCLEBREAK:
	case BOTH_LK_S_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_S_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_S_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_S_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_S_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_S_S_B_1_W:	//normal break I won
	case BOTH_LK_S_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_S_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_S_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_S_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_S_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_S_T_B_1_W:	//normal break I won
		return (PM_SuperBreakLoseAnim(anim)||PM_SuperBreakWinAnim(anim));
		break;
	}
	return qfalse;
}

qboolean PM_GetupAnimNoMove( int legsAnim )
{
	switch( legsAnim )
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_KnockDownAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	/*
	//special anims:
	case BOTH_RELEASED:
	case BOTH_LK_DL_ST_T_SB_1_L:
	case BOTH_PLAYER_PA_3_FLY:
	*/
		return qtrue;
		break;
	/*
	default:
		return PM_InGetUp( ps );
		break;
	*/
	}
	return qfalse;
}

qboolean PM_KnockDownAnimExtended( int anim )
{
	switch ( anim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	//special anims:
	case BOTH_RELEASED:
	case BOTH_LK_DL_ST_T_SB_1_L:
	case BOTH_PLAYER_PA_3_FLY:
		return qtrue;
		break;
	/*
	default:
		return PM_InGetUp( ps );
		break;
	*/
	}
	return qfalse;
}

qboolean PM_SaberInKata( saberMoveName_t saberMove )
{
	switch ( saberMove )
	{
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
		return qtrue;
	default:
		break;
	}
	return qfalse;
}

qboolean PM_CanRollFromSoulCal( playerState_t *ps )
{
	if ( ps->legsAnim == BOTH_A7_SOULCAL
		&& ps->legsAnimTimer < 700
		&& ps->legsAnimTimer > 250 )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_FullBodyTauntAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_GESTURE1:
	case BOTH_DUAL_TAUNT:
	case BOTH_STAFF_TAUNT:
	case BOTH_BOW:
	case BOTH_MEDITATE:
	case BOTH_SHOWOFF_FAST:
	case BOTH_SHOWOFF_MEDIUM:
	case BOTH_SHOWOFF_STRONG:
	case BOTH_SHOWOFF_DUAL:
	case BOTH_SHOWOFF_STAFF:
	case BOTH_VICTORY_FAST:
	case BOTH_VICTORY_MEDIUM:
	case BOTH_VICTORY_STRONG:
	case BOTH_VICTORY_DUAL:
	case BOTH_VICTORY_STAFF:
		return qtrue;
		break;
	}
	return qfalse;
}
