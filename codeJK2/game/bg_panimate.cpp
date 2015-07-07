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

// define GAME_INCLUDE so that g_public.h does not define the
// short, server-visible gclient_t and gentity_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "../../code/qcommon/q_shared.h"
#include "b_local.h"
#include "g_shared.h"
#include "bg_local.h"
#include "../cgame/cg_local.h"
#include "anims.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "wp_saber.h"

extern pmove_t	*pm;
extern pml_t	pml;
extern cvar_t	*g_ICARUSDebug;
extern cvar_t	*g_timescale;
extern cvar_t	*g_synchSplitAnims;
extern cvar_t	*g_saberAnimSpeed;
extern cvar_t	*g_saberAutoAim;

extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

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

int PM_AnimLength( int index, animNumber_t anim );
// Okay, here lies the much-dreaded Pat-created FSM movement chart...  Heretic II strikes again!
// Why am I inflicting this on you?  Well, it's better than hardcoded states.
// Ideally this will be replaced with an external file or more sophisticated move-picker
// once the game gets out of prototype stage.

// Silly, but I'm replacing these macros so they are shorter!
#define AFLAG_IDLE	(SETANIM_FLAG_NORMAL)
#define AFLAG_ACTIVE (SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_WAIT (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_FINISH (SETANIM_FLAG_HOLD)

saberMoveData_t	saberMoveData[LS_MOVE_MAX] = {//							NB:randomized
	// name			anim				startQ	endQ	setanimflag		blend,	blocking	chain_idle		chain_attack	trailLen
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
	{"Lunge Att",	BOTH_LUNGE2_B__T_,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_LUNGE
	{"Jump Att",	BOTH_FORCELEAP2_T__B_,Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_JUMP_T__B_
	{"Flip Stab",	BOTH_JUMPFLIPSTABDOWN,Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200	},	// LS_A_FLIP_STAB
	{"Flip Slash",	BOTH_JUMPFLIPSLASHDOWN1,Q_L,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R_T_,	200	},	// LS_A_FLIP_SLASH

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
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__R_BR,//46
		LS_NONE,	//Can't transition to same pos!
		LS_T1__R_TR,
		LS_T1__R_T_,
		LS_T1__R_TL,
		LS_T1__R__L,
		LS_T1__R_BL,
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TR_BR,//52
		LS_T1_TR__R,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_TR_T_,
		LS_T1_TR_TL,
		LS_T1_TR__L,
		LS_T1_TR_BL,
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_T__BR,//58
		LS_T1_T___R,
		LS_T1_T__TR,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_T__TL,
		LS_T1_T___L,
		LS_T1_T__BL,
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TL_BR,//64
		LS_T1_TL__R,
		LS_T1_TL_TR,
		LS_T1_TL_T_,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_TL__L,
		LS_T1_TL_BL,
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__L_BR,//70
		LS_T1__L__R,
		LS_T1__L_TR,
		LS_T1__L_T_,
		LS_T1__L_TL,
		LS_NONE,	//Can't transition to same pos!
		LS_T1__L_BL,
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR,//76
		LS_T1_BL__R,
		LS_T1_BL_TR,
		LS_T1_BL_T_,
		LS_T1_BL_TL,
		LS_T1_BL__L,
		LS_NONE,	//Can't transition to same pos!
		LS_NONE,	//No transitions to bottom, and no anims start there, so shouldn't need any
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
	return FORCE_LEVEL_0;
}

int PM_PowerLevelForSaberAnim( playerState_t *ps )
{
	int anim = ps->torsoAnim;
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
		return FORCE_LEVEL_2;
	}
	if ( anim >= BOTH_P1_S1_T_ && anim <= BOTH_H1_S1_BR )
	{//parries, knockaways and broken parries
		return FORCE_LEVEL_1;//FIXME: saberAnimLevel?
	}
	switch ( anim )
	{
	case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_K1_S1_T_:	//# knockaway saber top
	case BOTH_K1_S1_TR:	//# knockaway saber top right
	case BOTH_K1_S1_TL:	//# knockaway saber top left
	case BOTH_K1_S1_BL:	//# knockaway saber bottom left
	case BOTH_K1_S1_B_:	//# knockaway saber bottom
	case BOTH_K1_S1_BR:	//# knockaway saber bottom right
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
		return FORCE_LEVEL_3;
		break;
	case BOTH_JUMPFLIPSLASHDOWN1://FIXME: only middle of anim should have any power
	case BOTH_JUMPFLIPSTABDOWN://FIXME: only middle of anim should have any power
		if ( ps->torsoAnimTimer <= 300 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( PM_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, (animNumber_t)anim ) - ps->torsoAnimTimer < 300 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
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
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
		return qtrue;
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
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
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
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
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
	if ( move >= LS_R_TL2BR && move <= LS_R_TL2BR )
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
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
		return qtrue;
	}
	return qfalse;
}

int PM_BrokenParryForAttack( int move )
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

int PM_BrokenParryForParry( int move )
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

int PM_KnockawayForParry( int move )
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

int PM_SaberBounceForAttack( int move )
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
	/*
	if ( pm->gent && pm->gent->client )
	{
		if ( pm->gent->client->NPC_class == CLASS_DESANN ||
			pm->gent->client->NPC_class == CLASS_TAVION )
		{//desann and tavion can link up as many attacks as they want
			return qfalse;
		}
	}
	*/
	if ( pm->ps->saberAnimLevel > FORCE_LEVEL_3 )
	{//desann and tavion can link up as many attacks as they want
		return qfalse;
	}
	//FIXME: instead of random, apply some sort of logical conditions to whether or
	//		not you can chain?  Like if you were completely missed, you can't chain as much, or...?
	//		And/Or based on FP_SABER_OFFENSE level?  So number of attacks you can chain
	//		increases with your FP_SABER_OFFENSE skill?
	if ( pm->ps->saberAnimLevel == FORCE_LEVEL_3 )
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
		if ( pm->ps->saberAnimLevel == FORCE_LEVEL_2 && pm->ps->saberAttackChainCount > Q_irand( 2, 5 ) )
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
	if ( !pm->ps->clientNum && !g_saberAutoAim->integer && pm->cmd.forwardmove >= 0 )
	{//don't auto-backstab
		return qfalse;
	}
	trace_t	trace;
	vec3_t end, fwd, fwdAngles = {0,pm->ps->viewangles[YAW],0};

	AngleVectors( fwdAngles, fwd, NULL, NULL );
	VectorMA( pm->ps->origin, -backCheckDist, fwd, end );

	pm->trace( &trace, pm->ps->origin, vec3_origin, vec3_origin, end, pm->ps->clientNum, CONTENTS_SOLID|CONTENTS_BODY, G2_NOCOLLIDE, 0 );
	if ( trace.fraction < 1.0f && trace.entityNum < ENTITYNUM_WORLD )
	{
		gentity_t *traceEnt = &g_entities[trace.entityNum];
		if ( traceEnt
			&& traceEnt->health > 0
			&& traceEnt->client
			&& traceEnt->client->playerTeam == pm->gent->client->enemyTeam
			&& traceEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( !pm->ps->clientNum )
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

int PM_PickBackStab( void )
{
	if ( !pm->gent || !pm->gent->client )
	{
		return LS_READY;
	}
	if ( pm->gent->client->NPC_class == CLASS_TAVION )
	{
		return LS_A_BACKSTAB;
	}
	else if ( pm->gent->client->NPC_class == CLASS_DESANN )
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
	else if ( pm->ps->saberAnimLevel == FORCE_LEVEL_2 )
	{//using medium attacks
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
extern int PM_NPCSaberAttackFromQuad( int quad );
int PM_SaberFlipOverAttackMove( void );
int PM_AttackForEnemyPos( qboolean allowFB )
{
	int autoMove = -1;

	vec3_t enemy_org, enemyDir, faceFwd, faceRight, faceUp, facingAngles = {0, pm->ps->viewangles[YAW], 0};
	AngleVectors( facingAngles, faceFwd, faceRight, faceUp );
	//FIXME: predict enemy position?
	if ( pm->gent->enemy->client )
	{
		VectorCopy( pm->gent->enemy->currentOrigin, enemy_org );
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
	float enemyDist = VectorNormalize( enemyDir );
	float dot = DotProduct( enemyDir, faceFwd );
	if ( dot > 0 )
	{//enemy is in front
		if ( (!pm->ps->clientNum || PM_ControlledByPlayer())
			&& dot > 0.65f
			&& pm->gent->enemy->client && PM_InKnockDownOnGround( &pm->gent->enemy->client->ps )
			&& enemyDir[2] <= 20 )
		{//guy is on the ground below me, do a top-down attack
			return LS_A_T2B;
		}
		if ( allowFB )
		{//directly in front anim allowed
			if ( enemyDist > 200 || pm->gent->enemy->health <= 0 )
			{//hmm, look in back for an enemy
				if ( pm->ps->clientNum && !PM_ControlledByPlayer() )
				{//player should never do this automatically
					if ( pm->gent && pm->gent->client && pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG && Q_irand( 0, pm->gent->NPC->rank ) > RANK_ENSIGN )
					{//only fencers and higher can do this, higher rank does it more
						if ( PM_CheckEnemyInBack( 100 ) )
						{
							return PM_PickBackStab();
						}
					}
				}
			}
			//this is the default only if they're *right* in front...
			if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) || ((!pm->ps->clientNum||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
			{
				if ( (pm->ps->saberAnimLevel == FORCE_LEVEL_2 || pm->ps->saberAnimLevel == FORCE_LEVEL_5)//using medium attacks or Tavion
					//&& !PM_InKnockDown( pm->ps )
					&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //can force jump
					&& !(pm->gent->flags&FL_LOCK_PLAYER_WEAPONS) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
					&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=500) //on ground or just jumped
					&& ( pm->ps->clientNum || pm->ps->legsAnim == BOTH_JUMP1 || pm->ps->legsAnim == BOTH_FORCEJUMP1 || pm->ps->legsAnim == BOTH_INAIR1 || pm->ps->legsAnim == BOTH_FORCEINAIR1 )//either an NPC or in a non-flip forward jump
					&& ( (pm->ps->clientNum&&!PM_ControlledByPlayer()&&!Q_irand(0,2)) || pm->cmd.upmove || (pm->ps->pm_flags&PMF_JUMPING) ) )//jumping
				{//flip over-forward down-attack
					if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) ||
						(pm->gent->NPC && (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT) && !Q_irand(0, 2) ) )
					{//only player or acrobat or boss and higher can do this
						if ( pm->gent->enemy->health > 0
							&& pm->gent->enemy->maxs[2] > 12
							&& (!pm->gent->enemy->client || !PM_InKnockDownOnGround( &pm->gent->enemy->client->ps ) )
							&& DistanceSquared( pm->gent->currentOrigin, enemy_org ) < 10000
							&& InFront( enemy_org, pm->gent->currentOrigin, facingAngles, 0.3f ) )
						{//enemy must be close and in front
							return PM_SaberFlipOverAttackMove();
						}
					}
				}
			}
			if ( pm->ps->clientNum && !PM_ControlledByPlayer())
			{//NPC
				if ( pm->gent->NPC
					&& pm->gent->NPC->rank >= RANK_LT_JG
					&& ( pm->gent->NPC->rank == RANK_LT_JG || Q_irand( 0, pm->gent->NPC->rank ) >= RANK_ENSIGN )
					//&& !PM_InKnockDown( pm->ps )
					&& ((pm->ps->saberAnimLevel == FORCE_LEVEL_1 && !Q_irand( 0, 2 ))
						||(pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_DESANN && !Q_irand( 0, 4 ))) )
				{//fencers and higher only - 33% chance of using lunge (desann 20% chance)
					autoMove = LS_A_LUNGE;
				}
				else
				{
					autoMove = LS_A_T2B;
				}
			}
			else
			{//player
				if ( pm->ps->saberAnimLevel == FORCE_LEVEL_1
					//&& !PM_InKnockDown( pm->ps )
					&& (pm->ps->pm_flags&PMF_DUCKED||pm->cmd.upmove<0) )
				{//ducking player
					autoMove = LS_A_LUNGE;
				}
				else
				{
					autoMove = LS_A_T2B;
				}
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
		if ( !pm->gent->enemy->client || pm->gent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( dot < -0.75f
				&& enemyDist < 128
				&& (pm->ps->saberAnimLevel == FORCE_LEVEL_1 || (pm->gent->client &&pm->gent->client->NPC_class == CLASS_TAVION&&Q_irand(0,2))) )
			{//fast back-stab
				if ( !(pm->ps->pm_flags&PMF_DUCKED) && pm->cmd.upmove >= 0 )
				{//can't do it while ducked?
					if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) || (pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG) )
					{//only fencers and above can do this
						autoMove = LS_A_BACKSTAB;
					}
				}
			}
			else if ( pm->ps->saberAnimLevel > FORCE_LEVEL_1 )
			{//higher level back spin-attacks
				if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) || ((!pm->ps->clientNum||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
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
	return autoMove;
}

int PM_SaberLungeAttackMove( void )
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

int PM_SaberJumpAttackMove( void )
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

int PM_SaberFlipOverAttackMove( void )
{
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
	if ( Q_irand( 0, 1 ) )
	{
		return LS_A_FLIP_STAB;
	}
	else
	{
		return LS_A_FLIP_SLASH;
	}
}

int PM_SaberAttackForMovement( int forwardmove, int rightmove, int move )
{
	if ( rightmove > 0 )
	{//moving right
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
	else if ( rightmove < 0 )
	{//moving left
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
	else
	{//not moving left or right
		if ( forwardmove > 0 )
		{//forward= T2B slash
			if ( pm->gent && pm->gent->enemy && pm->gent->enemy->client )
			{//I have an active enemy
				if ( !pm->ps->clientNum || PM_ControlledByPlayer() )
				{//a player who is running at an enemy
					//if the enemy is not a jedi, don't use top-down, pick a diagonal or side attack
					if ( pm->gent->enemy->s.weapon != WP_SABER && g_saberAutoAim->integer )
					{
						int autoMove = PM_AttackForEnemyPos( qfalse );
						if ( autoMove != -1 )
						{
							return autoMove;
						}
					}
				}
				if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) || ((!pm->ps->clientNum||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
				{
					if ( (pm->ps->saberAnimLevel == FORCE_LEVEL_2 || pm->ps->saberAnimLevel == FORCE_LEVEL_5)//using medium attacks or Tavion
						//&& !PM_InKnockDown( pm->ps )
						&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //can force jump
						&& !(pm->gent->flags&FL_LOCK_PLAYER_WEAPONS) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
						&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=500) //on ground or just jumped
						&& ( pm->ps->clientNum || pm->ps->legsAnim == BOTH_JUMP1 || pm->ps->legsAnim == BOTH_FORCEJUMP1 || pm->ps->legsAnim == BOTH_INAIR1 || pm->ps->legsAnim == BOTH_FORCEINAIR1 )//either an NPC or in a non-flip forward jump
						&& ((pm->ps->clientNum&&!PM_ControlledByPlayer()&&!Q_irand(0,2))||pm->cmd.upmove>0||pm->ps->pm_flags&PMF_JUMPING))//jumping
					{//flip over-forward down-attack
						if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) ||
							(pm->gent->NPC && (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT) && !Q_irand(0, 2) ) )
						{//only player or acrobat or boss and higher can do this
							vec3_t fwdAngles = {0,pm->ps->viewangles[YAW],0};
							if ( pm->gent->enemy->health > 0
								&& pm->gent->enemy->maxs[2] > 12
								&& (!pm->gent->enemy->client || !PM_InKnockDownOnGround( &pm->gent->enemy->client->ps ) )
								&& DistanceSquared( pm->gent->currentOrigin, pm->gent->enemy->currentOrigin ) < 10000
								&& InFront( pm->gent->enemy->currentOrigin, pm->gent->currentOrigin, fwdAngles, 0.3f ) )
							{//enemy must be alive, not low to ground, close and in front
								return PM_SaberFlipOverAttackMove();
							}
						}
					}
				}
			}
			if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) || ((!pm->ps->clientNum||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
			{
				if ( (pm->ps->saberAnimLevel == FORCE_LEVEL_1 || (pm->gent&&pm->gent->client&&pm->gent->client->NPC_class == CLASS_DESANN && !Q_irand( 0, 2 )))
					//&& !PM_InKnockDown( pm->ps )
					&& (pm->cmd.upmove < 0 || pm->ps->pm_flags&PMF_DUCKED)
					&& (pm->ps->legsAnim == BOTH_STAND2||pm->ps->legsAnim == BOTH_SABERFAST_STANCE||pm->ps->legsAnim == BOTH_SABERSLOW_STANCE||level.time-pm->ps->lastStationary<=500)  )
				{//not moving (or just started), ducked and using fast attacks
					if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) ||
						( pm->gent
							&& pm->gent->NPC
							&& pm->gent->NPC->rank >= RANK_LT_JG
							&& ( pm->gent->NPC->rank == RANK_LT_JG || Q_irand( 0, pm->gent->NPC->rank ) >= RANK_ENSIGN )
							&& !Q_irand( 0, 3-g_spskill->integer ) ) )
					{//only player or fencer and higher can do this
						return PM_SaberLungeAttackMove();
					}
				}
			}
			if ( pm->ps->clientNum && !PM_ControlledByPlayer() )
			{
				if ( (pm->ps->saberAnimLevel == FORCE_LEVEL_3 || (pm->gent&&pm->gent->client&&pm->gent->client->NPC_class == CLASS_DESANN && !Q_irand( 0, 1 )))//using strong attacks
					//&& !PM_InKnockDown( pm->ps )
					&& pm->ps->forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //can force jump
					&& pm->gent && !(pm->gent->flags&FL_LOCK_PLAYER_WEAPONS) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
					&& (pm->ps->groundEntityNum != ENTITYNUM_NONE||level.time-pm->ps->lastOnGround<=500) //on ground or just jumped (if not player)
					//&& (pm->ps->legsAnim == BOTH_STAND2||pm->ps->legsAnim == BOTH_SABERFAST_STANCE||pm->ps->legsAnim == BOTH_SABERSLOW_STANCE||level.time-pm->ps->lastStationary<=500)//standing or just started moving
					&& (pm->cmd.upmove||pm->ps->pm_flags&PMF_JUMPING))//jumping
				{//strong attack: jump-hack
					if ( //!pm->ps->clientNum ||
						(pm->gent && pm->gent->NPC && !PM_ControlledByPlayer() && (pm->gent->NPC->rank==RANK_CREWMAN||pm->gent->NPC->rank>=RANK_LT) ) )
					{//only player or acrobat or boss and higher can do this
						return PM_SaberJumpAttackMove();
					}
				}
			}

			return LS_A_T2B;
		}
		else if ( forwardmove < 0 )
		{//backward= T2B slash//B2T uppercut?
			if ( (pm->ps->clientNum&&!PM_ControlledByPlayer()) || ((!pm->ps->clientNum||PM_ControlledByPlayer()) && cg.renderingThirdPerson && !cg.zoomMode) )
			{
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
							&& (pm->ps->saberAnimLevel == FORCE_LEVEL_1 || (pm->gent->client &&pm->gent->client->NPC_class == CLASS_TAVION&&Q_irand(0,1))) )
						{//fast attacks and Tavion
							if ( !(pm->ps->pm_flags&PMF_DUCKED) && pm->cmd.upmove >= 0 )
							{//can't do it while ducked?
								if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) || (pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG) )
								{//only fencers and above can do this
									return LS_A_BACKSTAB;
								}
							}
						}
						else if ( pm->ps->saberAnimLevel > FORCE_LEVEL_1 )
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
						if ( (pm->ps->saberAnimLevel == FORCE_LEVEL_1
							|| pm->gent->client->NPC_class == CLASS_TAVION
							|| (pm->gent->client->NPC_class == CLASS_DESANN && !Q_irand( 0, 3 )))
							&& (enemyDistSq > 16384 || pm->gent->enemy->health <= 0) )//128 squared
						{//my enemy is pretty far in front of me and I'm using fast attacks
							if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) ||
								( pm->gent && pm->gent->client && pm->gent->NPC && pm->gent->NPC->rank >= RANK_LT_JG && Q_irand( 0, pm->gent->NPC->rank ) > RANK_ENSIGN ) )
							{//only fencers and higher can do this, higher rank does it more
								if ( PM_CheckEnemyInBack( 128 ) )
								{
									return PM_PickBackStab();
								}
							}
						}
						else if ( (pm->ps->saberAnimLevel >= FORCE_LEVEL_2
							|| pm->gent->client->NPC_class == CLASS_DESANN)
							&& (enemyDistSq > 40000 || pm->gent->enemy->health <= 0) )//200 squared
						{//enemy is very faw away and I'm using medium/strong attacks
							if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) ||
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
					if ( (!pm->ps->clientNum||PM_ControlledByPlayer()) && pm->gent && pm->gent->client )
					{//only player
						if ( PM_CheckEnemyInBack( 128 ) )
						{
							return PM_PickBackStab();
						}
					}
				}
			}
			//else just swing down
			return LS_A_T2B;
		}
		else if ( PM_SaberInBounce( move ) )
		{//bounces should go to their default attack if you don't specify a direction but are attacking
			int newmove;
			if ( pm->ps->clientNum && !PM_ControlledByPlayer() && Q_irand( 0, 3 ) )
			{//use NPC random
				newmove = PM_NPCSaberAttackFromQuad( saberMoveData[move].endQuad );
			}
			else
			{//player uses chain-attack
				newmove = saberMoveData[move].chain_attack;
			}
			if ( PM_SaberKataDone( move, newmove ) )
			{
				return saberMoveData[move].chain_idle;
			}
			else
			{
				return newmove;
			}
		}
		else if ( PM_SaberInKnockaway( move ) )
		{//bounces should go to their default attack if you don't specify a direction but are attacking
			int newmove;
			if ( pm->ps->clientNum && !PM_ControlledByPlayer() && Q_irand( 0, 3 ) )
			{//use NPC random
				newmove = PM_NPCSaberAttackFromQuad( saberMoveData[move].endQuad );
			}
			else
			{
				if ( pm->ps->saberAnimLevel == FORCE_LEVEL_1 ||
					pm->ps->saberAnimLevel == FORCE_LEVEL_5 )
				{//player is in fast attacks, so come right back down from the same spot
					newmove = PM_AttackMoveForQuad( saberMoveData[move].endQuad );
				}
				else
				{//use a transition to wrap to another attack from a different dir
					newmove = saberMoveData[move].chain_attack;
				}
			}
			if ( PM_SaberKataDone( move, newmove ) )
			{
				return saberMoveData[move].chain_idle;
			}
			else
			{
				return newmove;
			}
		}
		else if ( move == LS_READY || move == LS_A_FLIP_STAB || move == LS_A_FLIP_SLASH )
		{//Not moving at all, shouldn't have gotten here...?
			if ( pm->ps->clientNum || g_saberAutoAim->integer )
			{//auto-aim
				if ( pm->gent && pm->gent->enemy )
				{//based on enemy position, pick a proper attack
					int autoMove = PM_AttackForEnemyPos( qtrue );
					if ( autoMove != -1 )
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
				return Q_irand( LS_A_TL2BR, LS_A_T2B );
			}
		}
	}
	//FIXME: pick a return?
	return LS_NONE;
}

int PM_SaberAnimTransitionAnim( int curmove, int newmove )
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
				}
			}
			break;
		//transitioning to any other anim is not supported
		}
	}

	if ( retmove == LS_NONE )
	{
		return newmove;
	}

	return retmove;
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

	for ( int animation = 0; animation < FACE_TALK1; animation++ )
	{
		if ( animation >= TORSO_DROPWEAP1 && animation < LEGS_WALKBACK1 )
		{//not a possible legs anim
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

int PM_ValidateAnimRange( int startFrame, int endFrame, float animSpeed )
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

	for ( int animation = 0; animation < LEGS_WALKBACK1; animation++ )
	{
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

	return level.knownAnimFileSets[index].animations[anim].numFrames * fabs((double)(level.knownAnimFileSets[index].animations[anim].frameLerp));
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
	if ( g_saberAnimSpeed->value != 1.0f )
	{
		if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_CROUCHATTACKBACK1 )
		{
			*animSpeed *= g_saberAnimSpeed->value;
		}
	}
	if ( gent && gent->NPC && gent->NPC->rank == RANK_CIVILIAN )
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
		if ( !MatrixMode )
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
	if(!ValidAnimFileIndex(gent->client->clientInfo.animFileIndex))
	{
		return;
	}
	if ( anim < 0 || anim >= MAX_ANIMATIONS )
	{
		assert( 0&&"anim out of range!!!" );
#ifndef FINAL_BUILD
		G_Error( "%s tried to play invalid anim %d", gent->NPC_type, anim );
#endif
		return;
	}
	animation_t *animations = level.knownAnimFileSets[gent->client->clientInfo.animFileIndex].animations;
	float		timeScaleMod = PM_GetTimeScaleMod( gent );
	float		animSpeed, oldAnimSpeed;
	int			actualTime = (cg.time?cg.time:level.time);

	if ( gent && gent->client )
	{//lower offensive skill slows down the saber start attack animations
		PM_SaberStartTransAnim( gent->client->ps.saberAnimLevel, anim, &timeScaleMod, gent );
	//	PM_SaberStartTransAnim( anim, gent->s.number, gent->client->ps.forcePowerLevel[FP_SABER_OFFENSE], &timeScaleMod );
	}

	// Set torso anim
	if (setAnimParts & SETANIM_TORSO)
	{
		// or if a more important anim is running
		if( !(setAnimFlags & SETANIM_FLAG_OVERRIDE) && ((*torsoAnimTimer > 0)||(*torsoAnimTimer == -1)) )
		{
			goto setAnimLegs;
		}

		if ( !PM_HasAnimation( gent, anim ) )
		{
			if ( g_ICARUSDebug->integer >= 3  )
			{
				//gi.Printf(S_COLOR_RED"SET_ANIM_UPPER ERROR: anim %s does not exist in this model (%s)!\n", animTable[anim].name, ((gent!=NULL&&gent->client!=NULL) ? gent->client->renderInfo.torsoModelName : "unknown") );
			}
			goto setAnimLegs;
		}

		// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
		animSpeed = oldAnimSpeed = 50.0f / animations[anim].frameLerp * timeScaleMod;

		if ( gi.G2API_HaveWeGhoul2Models(gent->ghoul2) && gent->lowerLumbarBone != -1 )//gent->upperLumbarBone
		{//see if we need to tell ghoul2 to play it again because of a animSpeed change
			int		blah;
			float	junk;
			if (!gi.G2API_GetBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->lowerLumbarBone, actualTime, &junk, &blah, &blah, &blah, &oldAnimSpeed, NULL ))
			{
				animSpeed = oldAnimSpeed;
			}
		}

		if ( oldAnimSpeed == animSpeed )
		{//animSpeed has not changed
			// Don't reset if it's already running the anim
			if( !(setAnimFlags & SETANIM_FLAG_RESTART) && *torsoAnim == anim )
			{
				goto setAnimLegs;
			}
		}

		*torsoAnim = anim;

		// lets try and run a ghoul2 animation if we have a ghoul2 model on this guy
		if (gi.G2API_HaveWeGhoul2Models(gent->ghoul2))
		{
			if ( gent->lowerLumbarBone != -1 )//gent->upperLumbarBone
			{
				int startFrame, endFrame;

				if ( cg_debugAnim.integer == 3 || (!gent->s.number && cg_debugAnim.integer == 1) || (gent->s.number && cg_debugAnim.integer == 2) )
				{
					Com_Printf("Time=%d: %s TORSO anim %d %s\n", actualTime, gent->targetname, anim, animTable[anim].name );
				}


				// we have a ghoul2 model - animate it?
				// don't bother if the animation is missing
				if (!animations[anim].numFrames)
				{
					// remove it if we already have an animation on this bone
					if (gi.G2API_GetAnimRange(&gent->ghoul2[gent->playerModel], "lower_lumbar", &startFrame, &endFrame))//"upper_lumbar"
					{
#if G2_DEBUG_TIMING
						Com_Printf( "tstop %d\n", cg.time );
#endif
						gi.G2API_StopBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->lowerLumbarBone );//gent->upperLumbarBone
						if ( gent->motionBone != -1 )
						{
							gi.G2API_StopBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->motionBone );
						}
					}
				}
				else
				{

					// before we do this, lets see if this animation is one the legs are already playing?

					int	animFlags = BONE_ANIM_OVERRIDE_FREEZE;

					if (animations[anim].loopFrames != -1)
					{
						animFlags = BONE_ANIM_OVERRIDE_LOOP;
					}
					//HACKHACKHACK - need some better way of not lerping between something like a spin/flip and a death, etc.
					if ( blendTime > 0 )
					{
						animFlags |= BONE_ANIM_BLEND;
					}
					//HACKHACKHACK
					//qboolean animatingLegs =  gi.G2API_GetAnimRange(&gent->ghoul2[gent->playerModel], "model_root", &startFrame, &endFrame);
					float	currentFrame, legAnimSpeed, firstFrame, lastFrame;
					int		flags;
					qboolean animatingLegs = gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel],
										gent->rootBone, actualTime, &currentFrame,
										&startFrame, &endFrame, &flags, &legAnimSpeed, NULL );
					if ( g_synchSplitAnims->integer
						&& !(setAnimFlags & SETANIM_FLAG_RESTART)
						&& animatingLegs
						&& (animations[anim].firstFrame == startFrame)
						&& (legAnimSpeed == animSpeed )//|| oldAnimSpeed != animSpeed)
						&& (((animations[anim].numFrames ) + animations[anim].firstFrame) == endFrame))
					{//if we're playing this *exact* anim (speed check should fix problems with anims that play other anims backwards) on the legs already andwe're not restarting the anim, then match the legs' frame
						if ( 0 )
						{//just stop it
							gi.G2API_StopBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->lowerLumbarBone );//gent->upperLumbarBone
							gi.G2API_StopBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->motionBone );//gent->upperLumbarBone
						}
						else
						{//try to synch it
//							assert((currentFrame <=endFrame) && (currentFrame>=startFrame));
							// yes, its the same animation, so work out where we are in the leg anim, and blend us
#if G2_DEBUG_TIMING
							Com_Printf("tlegb %d     %d %d %4.2f %4.2f %d\n",
								actualTime,
								animations[anim].firstFrame,
								(animations[anim].numFrames  )+ animations[anim].firstFrame,
								legAnimSpeed,
								currentFrame,
								blendTime);
#endif
							if ( oldAnimSpeed != animSpeed
								&& ((oldAnimSpeed>0&&animSpeed>0) || (oldAnimSpeed<0&&animSpeed<0)) )
							{//match the new speed, actually
								legAnimSpeed = animSpeed;
							}
							if ( legAnimSpeed < 0 )
							{//play anim backwards
								lastFrame = animations[anim].firstFrame;// -1;
								firstFrame = (animations[anim].numFrames) + animations[anim].firstFrame;// -1) + animations[anim].firstFrame;
							}
							else
							{
								firstFrame = animations[anim].firstFrame;
								lastFrame = (animations[anim].numFrames ) + animations[anim].firstFrame;
							}
							gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->lowerLumbarBone, //gent->upperLumbarBone
														firstFrame, lastFrame, animFlags, legAnimSpeed,
														actualTime, currentFrame, blendTime);
							if ( gent->motionBone != -1 )
							{
								gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->motionBone,
														firstFrame, lastFrame, animFlags, legAnimSpeed,
														actualTime, currentFrame, blendTime);
							}
						}
					}
					else
					{// no, we aren't the same anim as the legs are running, so just run it.
						// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
						int	firstFrame;
						int	lastFrame;
						if ( animSpeed < 0 )
						{//play anim backwards
							lastFrame = animations[anim].firstFrame;// -1;
							firstFrame = (animations[anim].numFrames) + animations[anim].firstFrame;;// -1) + animations[anim].firstFrame;
						}
						else
						{
							firstFrame = animations[anim].firstFrame;
							lastFrame = (animations[anim].numFrames) + animations[anim].firstFrame;
						}
						// first decide if we are doing an animation on the torso already
						qboolean animatingTorso =  gi.G2API_GetAnimRange(&gent->ghoul2[gent->playerModel], "lower_lumbar", &startFrame, &endFrame);//"upper_lumbar"

						// lets see if a) we are already animating and b) we aren't going to do the same animation again
						if (animatingTorso && ( (firstFrame != startFrame) || (lastFrame != endFrame) ) )
						{
#if G2_DEBUG_TIMING
							Com_Printf("trsob %d     %d %d %4.2f %4.2f %d\n",
								actualTime,
								firstFrame,
								lastFrame,
								animSpeed,
								-1,
								blendTime);
#endif
							gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->lowerLumbarBone, //gent->upperLumbarBone
								firstFrame, lastFrame, animFlags,
								animSpeed, actualTime, -1, blendTime);

							if ( gent->motionBone != -1 )
							{
								gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->motionBone,
									firstFrame, lastFrame, animFlags,
									animSpeed, actualTime, -1, blendTime);
							}
						}
						else
						{
							// no, ok, no blend then because we are either looping on the same anim, or starting from no anim
#if G2_DEBUG_TIMING
							Com_Printf("trson %d     %d %d %4.2f %4.2f %d\n",
								actualTime,
								firstFrame,
								lastFrame,
								animSpeed,
								-1,
								-1);
#endif
							gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->lowerLumbarBone, //gent->upperLumbarBone
								firstFrame, lastFrame, animFlags&~BONE_ANIM_BLEND,
								animSpeed, cg.time, -1, -1);
							if ( gent->motionBone != -1 )
							{
								gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->motionBone,
									firstFrame, lastFrame, animFlags&~BONE_ANIM_BLEND,
									animSpeed, cg.time, -1, -1);
							}
						}
					}
				}
			}
		}
/*
#ifdef _DEBUG
		gi.Printf(S_COLOR_GREEN"SET_ANIM_UPPER: %s (%s)\n", animTable[anim].name, gent->targetname );
#endif
*/
		if ((gent->client) && (setAnimFlags & SETANIM_FLAG_HOLD))
		{//FIXME: allow to set a specific time?
			if ( timeScaleMod != 1.0 )
			{
				PM_SetTorsoAnimTimer( gent, torsoAnimTimer, (animations[anim].numFrames - 1) * fabs((double)animations[anim].frameLerp) / timeScaleMod );
			}
			else if ( setAnimFlags & SETANIM_FLAG_HOLDLESS )
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				int dur;

				dur = (animations[anim].numFrames -1) * fabs((double)animations[anim].frameLerp);
				//dur = ((int)(dur/50.0)) * 50;
				//dur -= blendTime;
				if (dur > 1)
				{
					PM_SetTorsoAnimTimer( gent, torsoAnimTimer, dur-1 );
				}
				else
				{
					PM_SetTorsoAnimTimer( gent, torsoAnimTimer, fabs((double)animations[anim].frameLerp) );
				}
			}
			else
			{
				PM_SetTorsoAnimTimer( gent, torsoAnimTimer, (animations[anim].numFrames ) * fabs((double)animations[anim].frameLerp) );
			}
		}
	}

setAnimLegs:
	// Set legs anim
	if (setAnimParts & SETANIM_LEGS)
	{
		// or if a more important anim is running
		if( !(setAnimFlags & SETANIM_FLAG_OVERRIDE) && ((*legsAnimTimer > 0)||(*legsAnimTimer == -1)) )
		{
			goto setAnimDone;
		}

		if ( !PM_HasAnimation( gent, anim ) )
		{
			if ( g_ICARUSDebug->integer >= 3 )
			{
				//gi.Printf(S_COLOR_RED"SET_ANIM_LOWER ERROR: anim %s does not exist in this model (%s)!\n", animTable[anim].name, ((gent!=NULL&&gent->client!=NULL) ? gent->client->renderInfo.legsModelName : "unknown") );
			}
			goto setAnimDone;
		}

		// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
		animSpeed = oldAnimSpeed = 50.0f / animations[anim].frameLerp * timeScaleMod;

		if ( gi.G2API_HaveWeGhoul2Models(gent->ghoul2) && gent->rootBone != -1 )
		{//see if we need to tell ghoul2 to play it again because of a animSpeed change
			int		blah;
			float	junk;
			if (!gi.G2API_GetBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->rootBone, actualTime, &junk, &blah, &blah, &blah, &oldAnimSpeed, NULL ))
			{
				animSpeed = oldAnimSpeed;
			}
		}

		if ( oldAnimSpeed == animSpeed )
		{//animSpeed has not changed
			// Don't reset if it's already running the anim
			if( !(setAnimFlags & SETANIM_FLAG_RESTART) && *legsAnim == anim )
			{
				goto setAnimDone;
			}
		}

		*legsAnim = anim;

		// lets try and run a ghoul2 animation if we have a ghoul2 model on this guy
		if (gi.G2API_HaveWeGhoul2Models(gent->ghoul2))
		{
			int startFrame, endFrame;

			if ( cg_debugAnim.integer == 3 || (!gent->s.number && cg_debugAnim.integer == 1) || (gent->s.number && cg_debugAnim.integer == 2) )
			{
				Com_Printf("Time=%d: %s LEGS anim %d %s\n", actualTime, gent->targetname, anim, animTable[anim].name );
			}

			int	animFlags = BONE_ANIM_OVERRIDE_FREEZE;

			if (animations[anim].loopFrames != -1)
			{
				animFlags = BONE_ANIM_OVERRIDE_LOOP;
			}
			//HACKHACKHACK - need some better way of not lerping between something like a spin/flip and a death, etc.
			if ( blendTime > 0 )
			{
				animFlags |= BONE_ANIM_BLEND;
			}
			//HACKHACKHACK

			// don't bother if the animation is missing
			if (!animations[anim].numFrames)
			{
				// remove it if we already have an animation on this bone
				if (gi.G2API_GetAnimRange(&gent->ghoul2[gent->playerModel], "model_root", &startFrame, &endFrame))
				{
#if G2_DEBUG_TIMING
					Com_Printf( "lstop %d\n", cg.time );
#endif
					gi.G2API_StopBoneAnimIndex( &gent->ghoul2[gent->playerModel], gent->rootBone );
				}
			}
			else
			{
				int	firstFrame;
				int	lastFrame;
				if ( animSpeed < 0 )
				{//play anim backwards
					lastFrame = animations[anim].firstFrame;// -1;
					firstFrame = (animations[anim].numFrames) + animations[anim].firstFrame;// -1) + animations[anim].firstFrame;
				}
				else
				{
					firstFrame = animations[anim].firstFrame;
					lastFrame = (animations[anim].numFrames ) + animations[anim].firstFrame;
				}
				//HACKHACKHACK
				//qboolean animatingTorso =  gi.G2API_GetAnimRangeIndex(&gent->ghoul2[gent->playerModel], gent->lowerLumbarBone, &startFrame, &endFrame);
				float	currentFrame, torsoAnimSpeed;
				int		flags;
				qboolean animatingTorso = gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->lowerLumbarBone,actualTime, &currentFrame, &startFrame, &endFrame, &flags, &torsoAnimSpeed, NULL);
				if ( g_synchSplitAnims->integer
					&& !(setAnimFlags & SETANIM_FLAG_RESTART)
					&& animatingTorso
					&& (torsoAnimSpeed == animSpeed)//|| oldAnimSpeed != animSpeed)
					&& (animations[anim].firstFrame == startFrame)
					&& (((animations[anim].numFrames ) + animations[anim].firstFrame) == endFrame))
				{//if we're playing this *exact* anim on the torso already and we're not restarting the anim, then match the torso's frame
					//try to synch it
//					assert((currentFrame <=endFrame) && (currentFrame>=startFrame));
					// yes, its the same animation, so work out where we are in the torso anim, and blend us
#if G2_DEBUG_TIMING
					Com_Printf("ltrsb %d     %d %d %4.2f %4.2f %d\n",
						actualTime,
						animations[anim].firstFrame,
						(animations[anim].numFrames  )+ animations[anim].firstFrame,
						legAnimSpeed,
						currentFrame,
						blendTime);
#endif
					if ( oldAnimSpeed != animSpeed
						&& ((oldAnimSpeed>0&&animSpeed>0) || (oldAnimSpeed<0&&animSpeed<0)) )
					{//match the new speed, actually
						torsoAnimSpeed = animSpeed;
					}
					if ( torsoAnimSpeed < 0 )
					{//play anim backwards
						lastFrame = animations[anim].firstFrame;// -1;
						firstFrame = (animations[anim].numFrames) + animations[anim].firstFrame;// -1) + animations[anim].firstFrame;
					}
					else
					{
						firstFrame = animations[anim].firstFrame;
						lastFrame = (animations[anim].numFrames ) + animations[anim].firstFrame;
					}

					gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->rootBone,
											firstFrame, lastFrame, animFlags, torsoAnimSpeed,
											actualTime, currentFrame, blendTime );
				}
				else
				{// no, we aren't the same anim as the torso is running, so just run it.
					// before we do this, lets see if this animation is one the legs are already playing?
					//qboolean animatingLegs =  gi.G2API_GetAnimRange(&gent->ghoul2[gent->playerModel], "model_root", &startFrame, &endFrame);
					float	currentFrame, legAnimSpeed;
					int		flags;
					qboolean animatingLegs = gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel],
										gent->rootBone, actualTime, &currentFrame,
										&startFrame, &endFrame, &flags, &legAnimSpeed, NULL );
					// lets see if a) we are already animating and b) we aren't going to do the same animation again
					if (animatingLegs
						&& ( (legAnimSpeed!=animSpeed) || (firstFrame != startFrame) || (lastFrame != endFrame) ) )
					{
#if G2_DEBUG_TIMING
						Com_Printf("legsb %d     %d %d %4.2f %4.2f %d\n",
							actualTime,
							firstFrame,
							lastFrame,
							animSpeed,
							-1,
							blendTime);
#endif
						gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->rootBone,
							firstFrame, lastFrame, animFlags,
							animSpeed, actualTime, -1, blendTime);
					}
					else
					{
						// no, ok, no blend then because we are either looping on the same anim, or starting from no anim
#if G2_DEBUG_TIMING
						Com_Printf("legsn %d     %d %d %4.2f %4.2f %d\n",
							actualTime,
							firstFrame,
							lastFrame,
							animSpeed,
							-1,
							-1);
#endif
						gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->rootBone,
							firstFrame, lastFrame, animFlags&~BONE_ANIM_BLEND,
							animSpeed, cg.time, -1, -1);
					}
				}
			}
		}

/*
#ifdef _DEBUG
		gi.Printf(S_COLOR_GREEN"SET_ANIM_LOWER: %s (%s)\n", animTable[anim].name, gent->targetname );
#endif
*/
		if ((gent->client) && (setAnimFlags & SETANIM_FLAG_HOLD))
		{//FIXME: allow to set a specific time?
			if ( timeScaleMod != 1.0 )
			{
				PM_SetLegsAnimTimer( gent, legsAnimTimer, (animations[anim].numFrames - 1) * fabs((double)animations[anim].frameLerp) / timeScaleMod );
			}
			else if ( setAnimFlags & SETANIM_FLAG_HOLDLESS )
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				int dur;

				dur = (animations[anim].numFrames -1) * fabs((double)animations[anim].frameLerp);
				//dur = ((int)(dur/50.0)) * 50;
				//dur -= blendTime;
				if (dur > 1)
				{
					PM_SetLegsAnimTimer( gent, legsAnimTimer, dur-1 );
				}
				else
				{
					PM_SetLegsAnimTimer( gent, legsAnimTimer, fabs((double)animations[anim].frameLerp) );
				}
			}
			else
			{
				PM_SetLegsAnimTimer( gent, legsAnimTimer, (animations[anim].numFrames ) * fabs((double)animations[anim].frameLerp) );
			}
		}
	}

setAnimDone:
	return;
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

/*
-------------------------
PM_TorsoAnimLightsaber
-------------------------
*/


// Note that this function is intended to set the animation for the player, but
// only does idle-ish anims.  Anything that has a timer associated, such as attacks and blocks,
// are set by PM_WeaponLightsaber()

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
		//PM_SetAnim( pm, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		return;
	}
	if ( pm->ps->forcePowersActive&(1<<FP_LIGHTNING) && pm->ps->forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
	{//holding an enemy aloft with force-grip
		//PM_SetAnim( pm, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		return;
	}

	if ( pm->ps->saberActive
		&& pm->ps->saberLength < 3
		&& !(pm->ps->saberEventFlags&SEF_HITWALL)
		&& pm->ps->weaponstate == WEAPON_RAISING )
	{
		PM_SetSaberMove(LS_DRAW);
		return;
	}
	else if ( !pm->ps->saberActive && pm->ps->saberLength )
	{
		PM_SetSaberMove(LS_PUTAWAY);
		return;
	}

	if (pm->ps->weaponTime > 0)
	{	// weapon is already busy.
		return;
	}

	if (	pm->ps->weaponstate == WEAPON_READY ||
			pm->ps->weaponstate == WEAPON_CHARGING ||
			pm->ps->weaponstate == WEAPON_CHARGING_ALT )
	{
		if ( pm->ps->weapon == WP_SABER && pm->ps->saberLength )
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
				PM_SetSaberMove( LS_READY );
			}
		}
		else if( pm->ps->legsAnim == BOTH_RUN1 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN1,SETANIM_FLAG_NORMAL);
			pm->ps->saberMove = LS_READY;
		}
		else if( pm->ps->legsAnim == BOTH_RUN2 )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN2,SETANIM_FLAG_NORMAL);
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
					if ( !pm->ps->clientNum && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
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
		if( pm->ps->legsAnim == BOTH_GUARD_LOOKAROUND1 )
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
			|| pm->ps->legsAnim == BOTH_STAND4IDLE1
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
			if ( pm->ps->saberInFlight && saberInAir )
			{
				if ( !PM_ForceAnim( pm->ps->torsoAnim ) || pm->ps->torsoAnimTimer < 300 )
				{//don't interrupt a force power anim
					PM_SetAnim( pm, SETANIM_TORSO,BOTH_SABERPULL,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
			}
			else
			{//saber is on
				// Idle for Lightsaber
				if ( pm->gent && pm->gent->client )
				{
					pm->gent->client->saberTrail.inAction = qfalse;
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
						if ( !pm->ps->clientNum && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
						{//using something
							if ( !pm->ps->useTime )
							{//stopped holding it, release
								PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							}//else still holding, leave it as it is
						}
						else
						{
							if ( PM_RunningAnim( pm->ps->legsAnim ) )
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
		PM_SetAnim(pm,SETANIM_BOTH,BOTH_GUNSIT1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
		return;
	}

	if ( pm->ps->taunting > level.time )
	{
		if ( PM_HasAnimation( pm->gent, BOTH_GESTURE1 ) )
		{
			PM_SetAnim(pm,SETANIM_BOTH,BOTH_GESTURE1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);//SETANIM_FLAG_NORMAL
			pm->gent->client->saberTrail.inAction = qtrue;
			pm->gent->client->saberTrail.duration = 100;
			//FIXME: will this reset?
			//FIXME: force-control (yellow glow) effect on hand and saber?
		}
		else
		{
			PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE1,SETANIM_FLAG_NORMAL);
		}
		return;
	}

	if (pm->ps->weapon == WP_SABER )		// WP_LIGHTSABER
	{
		if ( pm->ps->saberLength > 0 && !pm->ps->saberInFlight )
		{
			PM_TorsoAnimLightsaber();
		}
		else
		{
			if ( pm->ps->forcePowersActive&(1<<FP_GRIP) && pm->ps->forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
			{//holding an enemy aloft with force-grip
				//PM_SetAnim( pm, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				return;
			}
			if ( pm->ps->forcePowersActive&(1<<FP_LIGHTNING) && pm->ps->forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
			{//holding an enemy aloft with force-grip
				//PM_SetAnim( pm, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				return;
			}
			qboolean saberInAir = qtrue;

			if ( PM_SaberInBrokenParry( pm->ps->saberMove ) || pm->ps->saberBlocked == BLOCKED_PARRY_BROKEN || PM_DodgeAnim( pm->ps->torsoAnim ) )
			{//we're stuck in a broken parry
				PM_TorsoAnimLightsaber();
				return;
			}
			if ( pm->ps->saberEntityNum < ENTITYNUM_NONE && pm->ps->saberEntityNum > 0 )//player is 0
			{//
				if ( &g_entities[pm->ps->saberEntityNum] != NULL && g_entities[pm->ps->saberEntityNum].s.pos.trType == TR_STATIONARY )
				{//fell to the ground and we're not trying to pull it back
					saberInAir = qfalse;
				}
			}

			if ( pm->ps->saberInFlight && saberInAir )
			{
				if ( !PM_ForceAnim( pm->ps->torsoAnim ) || pm->ps->torsoAnimTimer < 300 )
				{//don't interrupt a force power anim
					PM_SetAnim( pm, SETANIM_TORSO,BOTH_SABERPULL,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
			}
			else
			{
				if ( PM_InSlopeAnim( pm->ps->legsAnim ) )
				{//HMM... this probably breaks the saber putaway and select anims
					if ( pm->ps->saberLength > 0 )
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
					if ( !pm->ps->clientNum && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
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
	else if ( pm->ps->weaponTime )
	{
		weaponBusy = qtrue;
	}
	else if ( pm->gent && pm->gent->client->fireDelay > 0 )
	{
		weaponBusy = qtrue;
	}
	else if ( !pm->ps->clientNum && cg.zoomTime > cg.time - 5000 )
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
		if ( pm->ps->weapon == WP_SABER && pm->ps->saberLength )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_ATTACK1,SETANIM_FLAG_NORMAL);//TORSO_WEAPONREADY1
		}
		else if( pm->ps->legsAnim == BOTH_RUN1 && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_RUN2 && !weaponBusy )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_RUN2,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_WALK1 && !weaponBusy  )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK1,SETANIM_FLAG_NORMAL);
		}
		else if( pm->ps->legsAnim == BOTH_WALK2 && !weaponBusy  )
		{
			PM_SetAnim(pm,SETANIM_TORSO,BOTH_WALK2,SETANIM_FLAG_NORMAL);
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
			if ( !pm->ps->clientNum && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
			{//using something
				if ( !pm->ps->useTime )
				{//stopped holding it, release
					PM_SetAnim( pm, SETANIM_TORSO, BOTH_BUTTON_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}//else still holding, leave it as it is
			}
			else if ( pm->gent != NULL
				&& pm->gent->s.number == 0
				&& pm->ps->weaponstate != WEAPON_CHARGING
				&& pm->ps->weaponstate != WEAPON_CHARGING_ALT )
			{//PLayer- temp hack for weapon frame
				if ( pm->ps->weapon == WP_MELEE )
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
					if ( weaponBusy )
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
						if ( pm->gent && pm->gent->client && !PM_DroidMelee( pm->gent->client->NPC_class ) )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND6,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
						}
					}
					break;
				case WP_BLASTER:
					PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_ATTACK2,SETANIM_FLAG_NORMAL);
					break;
				case WP_DISRUPTOR:
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
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );//TORSO_WEAPONREADY4//SETANIM_FLAG_RESTART|
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
						if ( !pm->ps->clientNum && (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_CHARGING_ALT) )
						{//player pulling back to throw
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
			|| pm->ps->legsAnim == BOTH_STAND4IDLE1
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
		else if ( !pm->ps->clientNum && pm->ps->torsoAnim == BOTH_BUTTON_HOLD )
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
				&& ( PM_RunningAnim( pm->ps->legsAnim )
					|| (PM_WalkingAnim( pm->ps->legsAnim ) && !pm->ps->clientNum)
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
					if ( weaponBusy )
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
						if ( pm->gent && pm->gent->client && !PM_DroidMelee( pm->gent->client->NPC_class ) )
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND6,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_NORMAL);
						}
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
							PM_SetAnim( pm, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );//TORSO_WEAPONREADY4//SETANIM_FLAG_RESTART|
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
					if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_GALAKMECH )
					{//
						if ( pm->gent->alt_fire )
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE1,SETANIM_FLAG_NORMAL);
						}
					}
					else
					{
						if ( weaponBusy )
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
						}
						else
						{
							PM_SetAnim(pm,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
						}
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
	case BOTH_STAND4IDLE1:		//# Random standing idle
	case BOTH_STAND5:			//# standing idle, no weapon, hand down, back straight
	case BOTH_STAND5IDLE1:		//# Random standing idle
	case BOTH_STAND6:			//# one handed: gun at side: relaxed stand
	case BOTH_STAND1TO3:			//# Transition from stand1 to stand3
	case BOTH_STAND3TO1:			//# Transition from stand3 to stand1
	case BOTH_STAND2TO4:			//# Transition from stand2 to stand4
	case BOTH_STAND4TO2:			//# Transition from stand4 to stand2
	case BOTH_STANDUP1:			//# standing up and stumbling
	case BOTH_GESTURE1:			//# Generic gesture: non-specific
	case BOTH_GESTURE2:			//# Generic gesture: non-specific
	case BOTH_GESTURE3:			//# Generic gesture: non-specific
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
	case BOTH_ATTACK5:			//# Attack with rocket launcher
	case BOTH_MELEE1:			//# First melee attack
	case BOTH_MELEE2:			//# Second melee attack
	case BOTH_MELEE3:			//# Third melee attack
	case BOTH_MELEE4:			//# Fourth melee attack
	case BOTH_MELEE5:			//# Fifth melee attack
	case BOTH_MELEE6:			//# Sixth melee attack
	case BOTH_COVERUP1_LOOP:		//# animation of getting in line of friendly fire
	case BOTH_GUARD_LOOKAROUND1:	//# Cradling weapon and looking around
	case BOTH_GUARD_IDLE1:		//# Cradling weapon and standing
	case BOTH_ALERT1:			//# Startled by something while on guard
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
	case BOTH_STAND4IDLE1:		//# Random standing idle
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
	case BOTH_CROUCH2IDLE:		//# crouch and resting on back righ heel: no weapon
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
	case BOTH_PAIN2WRITHE1:		//# Transition from upright position to writhing on ground anim
	case BOTH_WRITHING1:			//# Lying on ground writhing in pain
	case BOTH_WRITHING1RLEG:		//# Lying on ground writhing in pain: holding right leg
	case BOTH_WRITHING1LLEG:		//# Lying on ground writhing in pain: holding left leg
	case BOTH_WRITHING2:			//# Lying on stomache writhing in pain
	case BOTH_INJURED1:			//# Lying down: against wall - can also be sleeping
	case BOTH_CRAWLBACK1:			//# Lying on back: crawling backwards with elbows
	case BOTH_INJURED2:			//# Injured pose 2
	case BOTH_INJURED3:			//# Injured pose 3
	case BOTH_INJURED6:			//# Injured pose 6
	case BOTH_INJURED6ATTACKSTART:	//# Start attack while in injured 6 pose
	case BOTH_INJURED6ATTACKSTOP:	//# End attack while in injured 6 pose
	case BOTH_INJURED6COMBADGE:	//# Hit combadge while in injured 6 pose
	case BOTH_INJURED6POINT:		//# Chang points to door while in injured state
	case BOTH_SLEEP1:			//# laying on back-rknee up-rhand on torso
	case BOTH_SLEEP2:			//# on floor-back against wall-arms crossed
	case BOTH_SLEEP5:			//# Laying on side sleeping on flat sufrace
	case BOTH_SLEEP_IDLE1:		//# rub face and nose while asleep from sleep pose 1
	case BOTH_SLEEP_IDLE2:		//# shift position while asleep - stays in sleep2
	case BOTH_SLEEP1_NOSE:		//# Scratch nose from SLEEP1 pose
	case BOTH_SLEEP2_SHIFT:		//# Shift in sleep from SLEEP2 pose
		return qtrue;
		break;
	case BOTH_KNOCKDOWN1:		//#
	case BOTH_KNOCKDOWN2:		//#
	case BOTH_KNOCKDOWN3:		//#
	case BOTH_KNOCKDOWN4:		//#
	case BOTH_KNOCKDOWN5:		//#
		if ( ps->legsAnimTimer < 500 )
		{//pretty much horizontal by this point
			return qtrue;
		}
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
	case BOTH_DEADFLOP3:		//# React to being shot from Third Death finished pose
	case BOTH_DEADFLOP4:		//# React to being shot from Fourth Death finished pose
	case BOTH_DEADFLOP5:		//# React to being shot from Fifth Death finished pose
	case BOTH_DEADFORWARD1_FLOP:		//# React to being shot First thrown forward death finished pose
	case BOTH_DEADFORWARD2_FLOP:		//# React to being shot Second thrown forward death finished pose
	case BOTH_DEADBACKWARD1_FLOP:	//# React to being shot First thrown backward death finished pose
	case BOTH_DEADBACKWARD2_FLOP:	//# React to being shot Second thrown backward death finished pose
	case BOTH_LYINGDEAD1_FLOP:		//# React to being shot Killed lying down death finished pose
	case BOTH_STUMBLEDEAD1_FLOP:		//# React to being shot Stumble forward death finished pose
	case BOTH_FALLDEAD1_FLOP:	//# React to being shot Fall forward and splat death finished pose
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

qboolean PM_StandingAnim( int anim )
{//NOTE: does not check idles or special (cinematic) stands
	switch ( anim )
	{
	case BOTH_STAND1:
	case BOTH_STAND2:
	case BOTH_STAND3:
	case BOTH_STAND4:
		return qtrue;
		break;
	}
	return qfalse;
}
