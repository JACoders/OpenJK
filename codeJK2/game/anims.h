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

#ifndef __ANIMS_H__
#define __ANIMS_H__
// playerAnimations


typedef enum //# animNumber_e
{
	//=================================================
	//ANIMS IN WHICH UPPER AND LOWER OBJECTS ARE IN MD3
	//=================================================
	BOTH_1CRUFTFORGIL = 0,		//# G2 cannot have a reverse anim at beginning of file
	//# #sep BOTH_ DEATHS
	BOTH_DEATH1,		//# First Death anim
	BOTH_DEATH2,			//# Second Death anim
	BOTH_DEATH3,			//# Third Death anim
	BOTH_DEATH4,			//# Fourth Death anim
	BOTH_DEATH5,			//# Fifth Death anim
	BOTH_DEATH6,			//# Sixth Death anim
	BOTH_DEATH7,			//# Seventh Death anim
	BOTH_DEATH8,			//#
	BOTH_DEATH9,			//#
	BOTH_DEATH10,			//#
	BOTH_DEATH11,			//#
	BOTH_DEATH12,			//#
	BOTH_DEATH13,			//#
	BOTH_DEATH14,			//#
	BOTH_DEATH15,			//#
	BOTH_DEATH16,			//#
	BOTH_DEATH17,			//#
	BOTH_DEATH18,			//#
	BOTH_DEATH19,			//#
	BOTH_DEATH20,			//#
	BOTH_DEATH21,			//#
	BOTH_DEATH22,			//#
	BOTH_DEATH23,			//#
	BOTH_DEATH24,			//#
	BOTH_DEATH25,			//#

	BOTH_DEATHFORWARD1,		//# First Death in which they get thrown forward
	BOTH_DEATHFORWARD2,		//# Second Death in which they get thrown forward
	BOTH_DEATHFORWARD3,		//# Tavion's falling in cin# 23
	BOTH_DEATHBACKWARD1,	//# First Death in which they get thrown backward
	BOTH_DEATHBACKWARD2,	//# Second Death in which they get thrown backward

	BOTH_DEATH1IDLE,		//# Idle while close to death
	BOTH_LYINGDEATH1,		//# Death to play when killed lying down
	BOTH_STUMBLEDEATH1,		//# Stumble forward and fall face first death
	BOTH_FALLDEATH1,		//# Fall forward off a high cliff and splat death - start
	BOTH_FALLDEATH1INAIR,	//# Fall forward off a high cliff and splat death - loop
	BOTH_FALLDEATH1LAND,	//# Fall forward off a high cliff and splat death - hit bottom
	BOTH_DEATH_ROLL,		//# Death anim from a roll
	BOTH_DEATH_FLIP,		//# Death anim from a flip
	BOTH_DEATH_SPIN_90_R,	//# Death anim when facing 90 degrees right
	BOTH_DEATH_SPIN_90_L,	//# Death anim when facing 90 degrees left
	BOTH_DEATH_SPIN_180,	//# Death anim when facing backwards
	BOTH_DEATH_LYING_UP,	//# Death anim when lying on back
	BOTH_DEATH_LYING_DN,	//# Death anim when lying on front
	BOTH_DEATH_FALLING_DN,	//# Death anim when falling on face
	BOTH_DEATH_FALLING_UP,	//# Death anim when falling on back
	BOTH_DEATH_CROUCHED,	//# Death anim when crouched
	//# #sep BOTH_ DEAD POSES # Should be last frame of corresponding previous anims
	BOTH_DEAD1,				//# First Death finished pose
	BOTH_DEAD2,				//# Second Death finished pose
	BOTH_DEAD3,				//# Third Death finished pose
	BOTH_DEAD4,				//# Fourth Death finished pose
	BOTH_DEAD5,				//# Fifth Death finished pose
	BOTH_DEAD6,				//# Sixth Death finished pose
	BOTH_DEAD7,				//# Seventh Death finished pose
	BOTH_DEAD8,				//#
	BOTH_DEAD9,				//#
	BOTH_DEAD10,			//#
	BOTH_DEAD11,			//#
	BOTH_DEAD12,			//#
	BOTH_DEAD13,			//#
	BOTH_DEAD14,			//#
	BOTH_DEAD15,			//#
	BOTH_DEAD16,			//#
	BOTH_DEAD17,			//#
	BOTH_DEAD18,			//#
	BOTH_DEAD19,			//#
	BOTH_DEAD20,			//#
	BOTH_DEAD21,			//#
	BOTH_DEAD22,			//#
	BOTH_DEAD23,			//#
	BOTH_DEAD24,			//#
	BOTH_DEAD25,			//#
	BOTH_DEADFORWARD1,		//# First thrown forward death finished pose
	BOTH_DEADFORWARD2,		//# Second thrown forward death finished pose
	BOTH_DEADBACKWARD1,		//# First thrown backward death finished pose
	BOTH_DEADBACKWARD2,		//# Second thrown backward death finished pose
	BOTH_LYINGDEAD1,		//# Killed lying down death finished pose
	BOTH_STUMBLEDEAD1,		//# Stumble forward death finished pose
	BOTH_FALLDEAD1LAND,		//# Fall forward and splat death finished pose
	//# #sep BOTH_ DEAD TWITCH/FLOP # React to being shot from death poses
	BOTH_DEADFLOP1,		//# React to being shot from First Death finished pose
	BOTH_DEADFLOP2,		//# React to being shot from Second Death finished pose
	BOTH_DEADFLOP3,		//# React to being shot from Third Death finished pose
	BOTH_DEADFLOP4,		//# React to being shot from Fourth Death finished pose
	BOTH_DEADFLOP5,		//# React to being shot from Fifth Death finished pose
	BOTH_DEADFORWARD1_FLOP,		//# React to being shot First thrown forward death finished pose
	BOTH_DEADFORWARD2_FLOP,		//# React to being shot Second thrown forward death finished pose
	BOTH_DEADBACKWARD1_FLOP,	//# React to being shot First thrown backward death finished pose
	BOTH_DEADBACKWARD2_FLOP,	//# React to being shot Second thrown backward death finished pose
	BOTH_LYINGDEAD1_FLOP,		//# React to being shot Killed lying down death finished pose
	BOTH_STUMBLEDEAD1_FLOP,		//# React to being shot Stumble forward death finished pose
	BOTH_FALLDEAD1_FLOP,	//# React to being shot Fall forward and splat death finished pose
	BOTH_DISMEMBER_HEAD1,	//#
	BOTH_DISMEMBER_TORSO1,	//#
	BOTH_DISMEMBER_LLEG,	//#
	BOTH_DISMEMBER_RLEG,	//#
	BOTH_DISMEMBER_RARM,	//#
	BOTH_DISMEMBER_LARM,	//#
	//# #sep BOTH_ PAINS
	BOTH_PAIN1,				//# First take pain anim
	BOTH_PAIN2,				//# Second take pain anim
	BOTH_PAIN3,				//# Third take pain anim
	BOTH_PAIN4,				//# Fourth take pain anim
	BOTH_PAIN5,				//# Fifth take pain anim - from behind
	BOTH_PAIN6,				//# Sixth take pain anim - from behind
	BOTH_PAIN7,				//# Seventh take pain anim - from behind
	BOTH_PAIN8,				//# Eigth take pain anim - from behind
	BOTH_PAIN9,				//#
	BOTH_PAIN10,			//#
	BOTH_PAIN11,			//#
	BOTH_PAIN12,			//#
	BOTH_PAIN13,			//#
	BOTH_PAIN14,			//#
	BOTH_PAIN15,			//#
	BOTH_PAIN16,			//#
	BOTH_PAIN17,			//#
	BOTH_PAIN18,			//#
	BOTH_PAIN19,			//#
	BOTH_PAIN20,			//# GETTING SHOCKED

	//# #sep BOTH_ ATTACKS
	BOTH_ATTACK1,			//# Attack with stun baton
	BOTH_ATTACK2,			//# Attack with one-handed pistol
	BOTH_ATTACK2IDLE1,		//# Idle with one-handed pistol
	BOTH_ATTACK3,			//# Attack with blaster rifle
	BOTH_ATTACK4,			//# Attack with disruptor
	BOTH_ATTACK5,			//# Attack with bow caster
	BOTH_ATTACK6,			//# Attack with ???
	BOTH_ATTACK7,			//# Attack with ???
	BOTH_ATTACK8,			//# Attack with ???
	BOTH_ATTACK9,			//# Attack with rocket launcher
	BOTH_ATTACK10,			//# Attack with thermal det
	BOTH_ATTACK11,			//# Attack with laser trap
	BOTH_ATTACK12,			//# Attack with detpack
	BOTH_MELEE1,			//# First melee attack
	BOTH_MELEE2,			//# Second melee attack
	BOTH_MELEE3,			//# Third melee attack
	BOTH_MELEE4,			//# Fourth melee attack
	BOTH_MELEE5,			//# Fifth melee attack
	BOTH_MELEE6,			//# Sixth melee attack
	BOTH_THERMAL_READY,		//# pull back with thermal
	BOTH_THERMAL_THROW,		//# throw thermal
	//* #sep BOTH_ SABER ANIMS
	//Saber attack anims - power level 2
	BOTH_A1_T__B_,	//# Fast weak vertical attack top to bottom
	BOTH_A1__L__R,	//# Fast weak horizontal attack left to right
	BOTH_A1__R__L,	//# Fast weak horizontal attack right to left
	BOTH_A1_TL_BR,	//# Fast weak diagonal attack top left to botom right
	BOTH_A1_BR_TL,	//# Fast weak diagonal attack top left to botom right
	BOTH_A1_BL_TR,	//# Fast weak diagonal attack bottom left to top right
	BOTH_A1_TR_BL,	//# Fast weak diagonal attack bottom left to right
	//Saber Arc and Spin Transitions
	BOTH_T1_BR__R,	//# Fast arc bottom right to right
	BOTH_T1_BR_TL,	//# Fast weak spin bottom right to top left
	BOTH_T1_BR__L,	//# Fast weak spin bottom right to left
	BOTH_T1_BR_BL,	//# Fast weak spin bottom right to bottom left
	BOTH_T1__R_TR,	//# Fast arc right to top right
	BOTH_T1__R_TL,	//# Fast arc right to top left
	BOTH_T1__R__L,	//# Fast weak spin right to left
	BOTH_T1__R_BL,	//# Fast weak spin right to bottom left
	BOTH_T1_TR_BR,	//# Fast arc top right to bottom right
	BOTH_T1_TR_TL,	//# Fast arc top right to top left
	BOTH_T1_TR__L,	//# Fast arc top right to left
	BOTH_T1_TR_BL,	//# Fast weak spin top right to bottom left
	BOTH_T1_T__BR,	//# Fast arc top to bottom right
	BOTH_T1_T___R,	//# Fast arc top to right
	BOTH_T1_T__TR,	//# Fast arc top to top right
	BOTH_T1_T__TL,	//# Fast arc top to top left
	BOTH_T1_T___L,	//# Fast arc top to left
	BOTH_T1_T__BL,	//# Fast arc top to bottom left
	BOTH_T1_TL_BR,	//# Fast weak spin top left to bottom right
	BOTH_T1_TL_BL,	//# Fast arc top left to bottom left
	BOTH_T1__L_BR,	//# Fast weak spin left to bottom right
	BOTH_T1__L__R,	//# Fast weak spin left to right
	BOTH_T1__L_TL,	//# Fast arc left to top left
	BOTH_T1_BL_BR,	//# Fast weak spin bottom left to bottom right
	BOTH_T1_BL__R,	//# Fast weak spin bottom left to right
	BOTH_T1_BL_TR,	//# Fast weak spin bottom left to top right
	BOTH_T1_BL__L,	//# Fast arc bottom left to left
	//Saber Arc Transitions that use existing animations played backwards
	BOTH_T1_BR_TR,	//# Fast arc bottom right to top right		(use: BOTH_T1_TR_BR)
	BOTH_T1_BR_T_,	//# Fast arc bottom right to top			(use: BOTH_T1_T__BR)
	BOTH_T1__R_BR,	//# Fast arc right to bottom right			(use: BOTH_T1_BR__R)
	BOTH_T1__R_T_,	//# Fast ar right to top				(use: BOTH_T1_T___R)
	BOTH_T1_TR__R,	//# Fast arc top right to right			(use: BOTH_T1__R_TR)
	BOTH_T1_TR_T_,	//# Fast arc top right to top				(use: BOTH_T1_T__TR)
	BOTH_T1_TL__R,	//# Fast arc top left to right			(use: BOTH_T1__R_TL)
	BOTH_T1_TL_TR,	//# Fast arc top left to top right			(use: BOTH_T1_TR_TL)
	BOTH_T1_TL_T_,	//# Fast arc top left to top				(use: BOTH_T1_T__TL)
	BOTH_T1_TL__L,	//# Fast arc top left to left				(use: BOTH_T1__L_TL)
	BOTH_T1__L_TR,	//# Fast arc left to top right			(use: BOTH_T1_TR__L)
	BOTH_T1__L_T_,	//# Fast arc left to top				(use: BOTH_T1_T___L)
	BOTH_T1__L_BL,	//# Fast arc left to bottom left			(use: BOTH_T1_BL__L)
	BOTH_T1_BL_T_,	//# Fast arc bottom left to top			(use: BOTH_T1_T__BL)
	BOTH_T1_BL_TL,	//# Fast arc bottom left to top left		(use: BOTH_T1_TL_BL)
	//Saber Attack Start Transitions
	BOTH_S1_S1_T_,	//# Fast plain transition from stance1 to top-to-bottom Fast weak attack
	BOTH_S1_S1__L,	//# Fast plain transition from stance1 to left-to-right Fast weak attack
	BOTH_S1_S1__R,	//# Fast plain transition from stance1 to right-to-left Fast weak attack
	BOTH_S1_S1_TL,	//# Fast plain transition from stance1 to top-left-to-bottom-right Fast weak attack
	BOTH_S1_S1_BR,	//# Fast plain transition from stance1 to bottom-right-to-top-left Fast weak attack
	BOTH_S1_S1_BL,	//# Fast plain transition from stance1 to bottom-left-to-top-right Fast weak attack
	BOTH_S1_S1_TR,	//# Fast plain transition from stance1 to top-right-to-bottom-left Fast weak attack
	//Saber Attack Return Transitions
	BOTH_R1_B__S1,	//# Fast plain transition from top-to-bottom Fast weak attack to stance1
	BOTH_R1__L_S1,	//# Fast plain transition from left-to-right Fast weak attack to stance1
	BOTH_R1__R_S1,	//# Fast plain transition from right-to-left Fast weak attack to stance1
	BOTH_R1_TL_S1,	//# Fast plain transition from top-left-to-bottom-right Fast weak attack to stance1
	BOTH_R1_BR_S1,	//# Fast plain transition from bottom-right-to-top-left Fast weak attack to stance1
	BOTH_R1_BL_S1,	//# Fast plain transition from bottom-left-to-top-right Fast weak attack to stance1
	BOTH_R1_TR_S1,	//# Fast plain transition from top-right-to-bottom-left Fast weak attack
	//Saber Attack Bounces (first 4 frames of an attack, played backwards)
	BOTH_B1_BR___,	//# Bounce-back if attack from BR is blocked
	BOTH_B1__R___,	//# Bounce-back if attack from R is blocked
	BOTH_B1_TR___,	//# Bounce-back if attack from TR is blocked
	BOTH_B1_T____,	//# Bounce-back if attack from T is blocked
	BOTH_B1_TL___,	//# Bounce-back if attack from TL is blocked
	BOTH_B1__L___,	//# Bounce-back if attack from L is blocked
	BOTH_B1_BL___,	//# Bounce-back if attack from BL is blocked
	//Saber Attack Deflections (last 4 frames of an attack)
	BOTH_D1_BR___,	//# Deflection toward BR
	BOTH_D1__R___,	//# Deflection toward R
	BOTH_D1_TR___,	//# Deflection toward TR
	BOTH_D1_TL___,	//# Deflection toward TL
	BOTH_D1__L___,	//# Deflection toward L
	BOTH_D1_BL___,	//# Deflection toward BL
	BOTH_D1_B____,	//# Deflection toward B
	//Saber attack anims - power level 2
	BOTH_A2_T__B_,	//# Fast weak vertical attack top to bottom
	BOTH_A2__L__R,	//# Fast weak horizontal attack left to right
	BOTH_A2__R__L,	//# Fast weak horizontal attack right to left
	BOTH_A2_TL_BR,	//# Fast weak diagonal attack top left to botom right
	BOTH_A2_BR_TL,	//# Fast weak diagonal attack top left to botom right
	BOTH_A2_BL_TR,	//# Fast weak diagonal attack bottom left to top right
	BOTH_A2_TR_BL,	//# Fast weak diagonal attack bottom left to right
	//Saber Arc and Spin Transitions
	BOTH_T2_BR__R,	//# Fast arc bottom right to right
	BOTH_T2_BR_TL,	//# Fast weak spin bottom right to top left
	BOTH_T2_BR__L,	//# Fast weak spin bottom right to left
	BOTH_T2_BR_BL,	//# Fast weak spin bottom right to bottom left
	BOTH_T2__R_TR,	//# Fast arc right to top right
	BOTH_T2__R_TL,	//# Fast arc right to top left
	BOTH_T2__R__L,	//# Fast weak spin right to left
	BOTH_T2__R_BL,	//# Fast weak spin right to bottom left
	BOTH_T2_TR_BR,	//# Fast arc top right to bottom right
	BOTH_T2_TR_TL,	//# Fast arc top right to top left
	BOTH_T2_TR__L,	//# Fast arc top right to left
	BOTH_T2_TR_BL,	//# Fast weak spin top right to bottom left
	BOTH_T2_T__BR,	//# Fast arc top to bottom right
	BOTH_T2_T___R,	//# Fast arc top to right
	BOTH_T2_T__TR,	//# Fast arc top to top right
	BOTH_T2_T__TL,	//# Fast arc top to top left
	BOTH_T2_T___L,	//# Fast arc top to left
	BOTH_T2_T__BL,	//# Fast arc top to bottom left
	BOTH_T2_TL_BR,	//# Fast weak spin top left to bottom right
	BOTH_T2_TL_BL,	//# Fast arc top left to bottom left
	BOTH_T2__L_BR,	//# Fast weak spin left to bottom right
	BOTH_T2__L__R,	//# Fast weak spin left to right
	BOTH_T2__L_TL,	//# Fast arc left to top left
	BOTH_T2_BL_BR,	//# Fast weak spin bottom left to bottom right
	BOTH_T2_BL__R,	//# Fast weak spin bottom left to right
	BOTH_T2_BL_TR,	//# Fast weak spin bottom left to top right
	BOTH_T2_BL__L,	//# Fast arc bottom left to left
	//Saber Arc Transitions that use existing animations played backwards
	BOTH_T2_BR_TR,	//# Fast arc bottom right to top right		(use: BOTH_T2_TR_BR)
	BOTH_T2_BR_T_,	//# Fast arc bottom right to top			(use: BOTH_T2_T__BR)
	BOTH_T2__R_BR,	//# Fast arc right to bottom right			(use: BOTH_T2_BR__R)
	BOTH_T2__R_T_,	//# Fast ar right to top				(use: BOTH_T2_T___R)
	BOTH_T2_TR__R,	//# Fast arc top right to right			(use: BOTH_T2__R_TR)
	BOTH_T2_TR_T_,	//# Fast arc top right to top				(use: BOTH_T2_T__TR)
	BOTH_T2_TL__R,	//# Fast arc top left to right			(use: BOTH_T2__R_TL)
	BOTH_T2_TL_TR,	//# Fast arc top left to top right			(use: BOTH_T2_TR_TL)
	BOTH_T2_TL_T_,	//# Fast arc top left to top				(use: BOTH_T2_T__TL)
	BOTH_T2_TL__L,	//# Fast arc top left to left				(use: BOTH_T2__L_TL)
	BOTH_T2__L_TR,	//# Fast arc left to top right			(use: BOTH_T2_TR__L)
	BOTH_T2__L_T_,	//# Fast arc left to top				(use: BOTH_T2_T___L)
	BOTH_T2__L_BL,	//# Fast arc left to bottom left			(use: BOTH_T2_BL__L)
	BOTH_T2_BL_T_,	//# Fast arc bottom left to top			(use: BOTH_T2_T__BL)
	BOTH_T2_BL_TL,	//# Fast arc bottom left to top left		(use: BOTH_T2_TL_BL)
	//Saber Attack Start Transitions
	BOTH_S2_S1_T_,	//# Fast plain transition from stance1 to top-to-bottom Fast weak attack
	BOTH_S2_S1__L,	//# Fast plain transition from stance1 to left-to-right Fast weak attack
	BOTH_S2_S1__R,	//# Fast plain transition from stance1 to right-to-left Fast weak attack
	BOTH_S2_S1_TL,	//# Fast plain transition from stance1 to top-left-to-bottom-right Fast weak attack
	BOTH_S2_S1_BR,	//# Fast plain transition from stance1 to bottom-right-to-top-left Fast weak attack
	BOTH_S2_S1_BL,	//# Fast plain transition from stance1 to bottom-left-to-top-right Fast weak attack
	BOTH_S2_S1_TR,	//# Fast plain transition from stance1 to top-right-to-bottom-left Fast weak attack
	//Saber Attack Return Transitions
	BOTH_R2_B__S1,	//# Fast plain transition from top-to-bottom Fast weak attack to stance1
	BOTH_R2__L_S1,	//# Fast plain transition from left-to-right Fast weak attack to stance1
	BOTH_R2__R_S1,	//# Fast plain transition from right-to-left Fast weak attack to stance1
	BOTH_R2_TL_S1,	//# Fast plain transition from top-left-to-bottom-right Fast weak attack to stance1
	BOTH_R2_BR_S1,	//# Fast plain transition from bottom-right-to-top-left Fast weak attack to stance1
	BOTH_R2_BL_S1,	//# Fast plain transition from bottom-left-to-top-right Fast weak attack to stance1
	BOTH_R2_TR_S1,	//# Fast plain transition from top-right-to-bottom-left Fast weak attack
	//Saber Attack Bounces (first 4 frames of an attack, played backwards)
	BOTH_B2_BR___,	//# Bounce-back if attack from BR is blocked
	BOTH_B2__R___,	//# Bounce-back if attack from R is blocked
	BOTH_B2_TR___,	//# Bounce-back if attack from TR is blocked
	BOTH_B2_T____,	//# Bounce-back if attack from T is blocked
	BOTH_B2_TL___,	//# Bounce-back if attack from TL is blocked
	BOTH_B2__L___,	//# Bounce-back if attack from L is blocked
	BOTH_B2_BL___,	//# Bounce-back if attack from BL is blocked
	//Saber Attack Deflections (last 4 frames of an attack)
	BOTH_D2_BR___,	//# Deflection toward BR
	BOTH_D2__R___,	//# Deflection toward R
	BOTH_D2_TR___,	//# Deflection toward TR
	BOTH_D2_TL___,	//# Deflection toward TL
	BOTH_D2__L___,	//# Deflection toward L
	BOTH_D2_BL___,	//# Deflection toward BL
	BOTH_D2_B____,	//# Deflection toward B
	//Saber attack anims - power level 3
	BOTH_A3_T__B_,	//# Fast weak vertical attack top to bottom
	BOTH_A3__L__R,	//# Fast weak horizontal attack left to right
	BOTH_A3__R__L,	//# Fast weak horizontal attack right to left
	BOTH_A3_TL_BR,	//# Fast weak diagonal attack top left to botom right
	BOTH_A3_BR_TL,	//# Fast weak diagonal attack top left to botom right
	BOTH_A3_BL_TR,	//# Fast weak diagonal attack bottom left to top right
	BOTH_A3_TR_BL,	//# Fast weak diagonal attack bottom left to right
	//Saber Arc and Spin Transitions
	BOTH_T3_BR__R,	//# Fast arc bottom right to right
	BOTH_T3_BR_TL,	//# Fast weak spin bottom right to top left
	BOTH_T3_BR__L,	//# Fast weak spin bottom right to left
	BOTH_T3_BR_BL,	//# Fast weak spin bottom right to bottom left
	BOTH_T3__R_TR,	//# Fast arc right to top right
	BOTH_T3__R_TL,	//# Fast arc right to top left
	BOTH_T3__R__L,	//# Fast weak spin right to left
	BOTH_T3__R_BL,	//# Fast weak spin right to bottom left
	BOTH_T3_TR_BR,	//# Fast arc top right to bottom right
	BOTH_T3_TR_TL,	//# Fast arc top right to top left
	BOTH_T3_TR__L,	//# Fast arc top right to left
	BOTH_T3_TR_BL,	//# Fast weak spin top right to bottom left
	BOTH_T3_T__BR,	//# Fast arc top to bottom right
	BOTH_T3_T___R,	//# Fast arc top to right
	BOTH_T3_T__TR,	//# Fast arc top to top right
	BOTH_T3_T__TL,	//# Fast arc top to top left
	BOTH_T3_T___L,	//# Fast arc top to left
	BOTH_T3_T__BL,	//# Fast arc top to bottom left
	BOTH_T3_TL_BR,	//# Fast weak spin top left to bottom right
	BOTH_T3_TL_BL,	//# Fast arc top left to bottom left
	BOTH_T3__L_BR,	//# Fast weak spin left to bottom right
	BOTH_T3__L__R,	//# Fast weak spin left to right
	BOTH_T3__L_TL,	//# Fast arc left to top left
	BOTH_T3_BL_BR,	//# Fast weak spin bottom left to bottom right
	BOTH_T3_BL__R,	//# Fast weak spin bottom left to right
	BOTH_T3_BL_TR,	//# Fast weak spin bottom left to top right
	BOTH_T3_BL__L,	//# Fast arc bottom left to left
	//Saber Arc Transitions that use existing animations played backwards
	BOTH_T3_BR_TR,	//# Fast arc bottom right to top right		(use: BOTH_T3_TR_BR)
	BOTH_T3_BR_T_,	//# Fast arc bottom right to top			(use: BOTH_T3_T__BR)
	BOTH_T3__R_BR,	//# Fast arc right to bottom right			(use: BOTH_T3_BR__R)
	BOTH_T3__R_T_,	//# Fast ar right to top				(use: BOTH_T3_T___R)
	BOTH_T3_TR__R,	//# Fast arc top right to right			(use: BOTH_T3__R_TR)
	BOTH_T3_TR_T_,	//# Fast arc top right to top				(use: BOTH_T3_T__TR)
	BOTH_T3_TL__R,	//# Fast arc top left to right			(use: BOTH_T3__R_TL)
	BOTH_T3_TL_TR,	//# Fast arc top left to top right			(use: BOTH_T3_TR_TL)
	BOTH_T3_TL_T_,	//# Fast arc top left to top				(use: BOTH_T3_T__TL)
	BOTH_T3_TL__L,	//# Fast arc top left to left				(use: BOTH_T3__L_TL)
	BOTH_T3__L_TR,	//# Fast arc left to top right			(use: BOTH_T3_TR__L)
	BOTH_T3__L_T_,	//# Fast arc left to top				(use: BOTH_T3_T___L)
	BOTH_T3__L_BL,	//# Fast arc left to bottom left			(use: BOTH_T3_BL__L)
	BOTH_T3_BL_T_,	//# Fast arc bottom left to top			(use: BOTH_T3_T__BL)
	BOTH_T3_BL_TL,	//# Fast arc bottom left to top left		(use: BOTH_T3_TL_BL)
	//Saber Attack Start Transitions
	BOTH_S3_S1_T_,	//# Fast plain transition from stance1 to top-to-bottom Fast weak attack
	BOTH_S3_S1__L,	//# Fast plain transition from stance1 to left-to-right Fast weak attack
	BOTH_S3_S1__R,	//# Fast plain transition from stance1 to right-to-left Fast weak attack
	BOTH_S3_S1_TL,	//# Fast plain transition from stance1 to top-left-to-bottom-right Fast weak attack
	BOTH_S3_S1_BR,	//# Fast plain transition from stance1 to bottom-right-to-top-left Fast weak attack
	BOTH_S3_S1_BL,	//# Fast plain transition from stance1 to bottom-left-to-top-right Fast weak attack
	BOTH_S3_S1_TR,	//# Fast plain transition from stance1 to top-right-to-bottom-left Fast weak attack
	//Saber Attack Return Transitions
	BOTH_R3_B__S1,	//# Fast plain transition from top-to-bottom Fast weak attack to stance1
	BOTH_R3__L_S1,	//# Fast plain transition from left-to-right Fast weak attack to stance1
	BOTH_R3__R_S1,	//# Fast plain transition from right-to-left Fast weak attack to stance1
	BOTH_R3_TL_S1,	//# Fast plain transition from top-left-to-bottom-right Fast weak attack to stance1
	BOTH_R3_BR_S1,	//# Fast plain transition from bottom-right-to-top-left Fast weak attack to stance1
	BOTH_R3_BL_S1,	//# Fast plain transition from bottom-left-to-top-right Fast weak attack to stance1
	BOTH_R3_TR_S1,	//# Fast plain transition from top-right-to-bottom-left Fast weak attack
	//Saber Attack Bounces (first 4 frames of an attack, played backwards)
	BOTH_B3_BR___,	//# Bounce-back if attack from BR is blocked
	BOTH_B3__R___,	//# Bounce-back if attack from R is blocked
	BOTH_B3_TR___,	//# Bounce-back if attack from TR is blocked
	BOTH_B3_T____,	//# Bounce-back if attack from T is blocked
	BOTH_B3_TL___,	//# Bounce-back if attack from TL is blocked
	BOTH_B3__L___,	//# Bounce-back if attack from L is blocked
	BOTH_B3_BL___,	//# Bounce-back if attack from BL is blocked
	//Saber Attack Deflections (last 4 frames of an attack)
	BOTH_D3_BR___,	//# Deflection toward BR
	BOTH_D3__R___,	//# Deflection toward R
	BOTH_D3_TR___,	//# Deflection toward TR
	BOTH_D3_TL___,	//# Deflection toward TL
	BOTH_D3__L___,	//# Deflection toward L
	BOTH_D3_BL___,	//# Deflection toward BL
	BOTH_D3_B____,	//# Deflection toward B
	//Saber attack anims - power level 4  - Desann's
	BOTH_A4_T__B_,	//# Fast weak vertical attack top to bottom
	BOTH_A4__L__R,	//# Fast weak horizontal attack left to right
	BOTH_A4__R__L,	//# Fast weak horizontal attack right to left
	BOTH_A4_TL_BR,	//# Fast weak diagonal attack top left to botom right
	BOTH_A4_BR_TL,	//# Fast weak diagonal attack top left to botom right
	BOTH_A4_BL_TR,	//# Fast weak diagonal attack bottom left to top right
	BOTH_A4_TR_BL,	//# Fast weak diagonal attack bottom left to right
	//Saber Arc and Spin Transitions
	BOTH_T4_BR__R,	//# Fast arc bottom right to right
	BOTH_T4_BR_TL,	//# Fast weak spin bottom right to top left
	BOTH_T4_BR__L,	//# Fast weak spin bottom right to left
	BOTH_T4_BR_BL,	//# Fast weak spin bottom right to bottom left
	BOTH_T4__R_TR,	//# Fast arc right to top right
	BOTH_T4__R_TL,	//# Fast arc right to top left
	BOTH_T4__R__L,	//# Fast weak spin right to left
	BOTH_T4__R_BL,	//# Fast weak spin right to bottom left
	BOTH_T4_TR_BR,	//# Fast arc top right to bottom right
	BOTH_T4_TR_TL,	//# Fast arc top right to top left
	BOTH_T4_TR__L,	//# Fast arc top right to left
	BOTH_T4_TR_BL,	//# Fast weak spin top right to bottom left
	BOTH_T4_T__BR,	//# Fast arc top to bottom right
	BOTH_T4_T___R,	//# Fast arc top to right
	BOTH_T4_T__TR,	//# Fast arc top to top right
	BOTH_T4_T__TL,	//# Fast arc top to top left
	BOTH_T4_T___L,	//# Fast arc top to left
	BOTH_T4_T__BL,	//# Fast arc top to bottom left
	BOTH_T4_TL_BR,	//# Fast weak spin top left to bottom right
	BOTH_T4_TL_BL,	//# Fast arc top left to bottom left
	BOTH_T4__L_BR,	//# Fast weak spin left to bottom right
	BOTH_T4__L__R,	//# Fast weak spin left to right
	BOTH_T4__L_TL,	//# Fast arc left to top left
	BOTH_T4_BL_BR,	//# Fast weak spin bottom left to bottom right
	BOTH_T4_BL__R,	//# Fast weak spin bottom left to right
	BOTH_T4_BL_TR,	//# Fast weak spin bottom left to top right
	BOTH_T4_BL__L,	//# Fast arc bottom left to left
	//Saber Arc Transitions that use existing animations played backwards
	BOTH_T4_BR_TR,	//# Fast arc bottom right to top right		(use: BOTH_T4_TR_BR)
	BOTH_T4_BR_T_,	//# Fast arc bottom right to top			(use: BOTH_T4_T__BR)
	BOTH_T4__R_BR,	//# Fast arc right to bottom right			(use: BOTH_T4_BR__R)
	BOTH_T4__R_T_,	//# Fast ar right to top				(use: BOTH_T4_T___R)
	BOTH_T4_TR__R,	//# Fast arc top right to right			(use: BOTH_T4__R_TR)
	BOTH_T4_TR_T_,	//# Fast arc top right to top				(use: BOTH_T4_T__TR)
	BOTH_T4_TL__R,	//# Fast arc top left to right			(use: BOTH_T4__R_TL)
	BOTH_T4_TL_TR,	//# Fast arc top left to top right			(use: BOTH_T4_TR_TL)
	BOTH_T4_TL_T_,	//# Fast arc top left to top				(use: BOTH_T4_T__TL)
	BOTH_T4_TL__L,	//# Fast arc top left to left				(use: BOTH_T4__L_TL)
	BOTH_T4__L_TR,	//# Fast arc left to top right			(use: BOTH_T4_TR__L)
	BOTH_T4__L_T_,	//# Fast arc left to top				(use: BOTH_T4_T___L)
	BOTH_T4__L_BL,	//# Fast arc left to bottom left			(use: BOTH_T4_BL__L)
	BOTH_T4_BL_T_,	//# Fast arc bottom left to top			(use: BOTH_T4_T__BL)
	BOTH_T4_BL_TL,	//# Fast arc bottom left to top left		(use: BOTH_T4_TL_BL)
	//Saber Attack Start Transitions
	BOTH_S4_S1_T_,	//# Fast plain transition from stance1 to top-to-bottom Fast weak attack
	BOTH_S4_S1__L,	//# Fast plain transition from stance1 to left-to-right Fast weak attack
	BOTH_S4_S1__R,	//# Fast plain transition from stance1 to right-to-left Fast weak attack
	BOTH_S4_S1_TL,	//# Fast plain transition from stance1 to top-left-to-bottom-right Fast weak attack
	BOTH_S4_S1_BR,	//# Fast plain transition from stance1 to bottom-right-to-top-left Fast weak attack
	BOTH_S4_S1_BL,	//# Fast plain transition from stance1 to bottom-left-to-top-right Fast weak attack
	BOTH_S4_S1_TR,	//# Fast plain transition from stance1 to top-right-to-bottom-left Fast weak attack
	//Saber Attack Return Transitions
	BOTH_R4_B__S1,	//# Fast plain transition from top-to-bottom Fast weak attack to stance1
	BOTH_R4__L_S1,	//# Fast plain transition from left-to-right Fast weak attack to stance1
	BOTH_R4__R_S1,	//# Fast plain transition from right-to-left Fast weak attack to stance1
	BOTH_R4_TL_S1,	//# Fast plain transition from top-left-to-bottom-right Fast weak attack to stance1
	BOTH_R4_BR_S1,	//# Fast plain transition from bottom-right-to-top-left Fast weak attack to stance1
	BOTH_R4_BL_S1,	//# Fast plain transition from bottom-left-to-top-right Fast weak attack to stance1
	BOTH_R4_TR_S1,	//# Fast plain transition from top-right-to-bottom-left Fast weak attack
	//Saber Attack Bounces (first 4 frames of an attack, played backwards)
	BOTH_B4_BR___,	//# Bounce-back if attack from BR is blocked
	BOTH_B4__R___,	//# Bounce-back if attack from R is blocked
	BOTH_B4_TR___,	//# Bounce-back if attack from TR is blocked
	BOTH_B4_T____,	//# Bounce-back if attack from T is blocked
	BOTH_B4_TL___,	//# Bounce-back if attack from TL is blocked
	BOTH_B4__L___,	//# Bounce-back if attack from L is blocked
	BOTH_B4_BL___,	//# Bounce-back if attack from BL is blocked
	//Saber Attack Deflections (last 4 frames of an attack)
	BOTH_D4_BR___,	//# Deflection toward BR
	BOTH_D4__R___,	//# Deflection toward R
	BOTH_D4_TR___,	//# Deflection toward TR
	BOTH_D4_TL___,	//# Deflection toward TL
	BOTH_D4__L___,	//# Deflection toward L
	BOTH_D4_BL___,	//# Deflection toward BL
	BOTH_D4_B____,	//# Deflection toward B
	//Saber attack anims - power level 5  - Tavion's
	BOTH_A5_T__B_,	//# Fast weak vertical attack top to bottom
	BOTH_A5__L__R,	//# Fast weak horizontal attack left to right
	BOTH_A5__R__L,	//# Fast weak horizontal attack right to left
	BOTH_A5_TL_BR,	//# Fast weak diagonal attack top left to botom right
	BOTH_A5_BR_TL,	//# Fast weak diagonal attack top left to botom right
	BOTH_A5_BL_TR,	//# Fast weak diagonal attack bottom left to top right
	BOTH_A5_TR_BL,	//# Fast weak diagonal attack bottom left to right
	//Saber Arc and Spin Transitions
	BOTH_T5_BR__R,	//# Fast arc bottom right to right
	BOTH_T5_BR_TL,	//# Fast weak spin bottom right to top left
	BOTH_T5_BR__L,	//# Fast weak spin bottom right to left
	BOTH_T5_BR_BL,	//# Fast weak spin bottom right to bottom left
	BOTH_T5__R_TR,	//# Fast arc right to top right
	BOTH_T5__R_TL,	//# Fast arc right to top left
	BOTH_T5__R__L,	//# Fast weak spin right to left
	BOTH_T5__R_BL,	//# Fast weak spin right to bottom left
	BOTH_T5_TR_BR,	//# Fast arc top right to bottom right
	BOTH_T5_TR_TL,	//# Fast arc top right to top left
	BOTH_T5_TR__L,	//# Fast arc top right to left
	BOTH_T5_TR_BL,	//# Fast weak spin top right to bottom left
	BOTH_T5_T__BR,	//# Fast arc top to bottom right
	BOTH_T5_T___R,	//# Fast arc top to right
	BOTH_T5_T__TR,	//# Fast arc top to top right
	BOTH_T5_T__TL,	//# Fast arc top to top left
	BOTH_T5_T___L,	//# Fast arc top to left
	BOTH_T5_T__BL,	//# Fast arc top to bottom left
	BOTH_T5_TL_BR,	//# Fast weak spin top left to bottom right
	BOTH_T5_TL_BL,	//# Fast arc top left to bottom left
	BOTH_T5__L_BR,	//# Fast weak spin left to bottom right
	BOTH_T5__L__R,	//# Fast weak spin left to right
	BOTH_T5__L_TL,	//# Fast arc left to top left
	BOTH_T5_BL_BR,	//# Fast weak spin bottom left to bottom right
	BOTH_T5_BL__R,	//# Fast weak spin bottom left to right
	BOTH_T5_BL_TR,	//# Fast weak spin bottom left to top right
	BOTH_T5_BL__L,	//# Fast arc bottom left to left
	//Saber Arc Transitions that use existing animations played backwards
	BOTH_T5_BR_TR,	//# Fast arc bottom right to top right		(use: BOTH_T5_TR_BR)
	BOTH_T5_BR_T_,	//# Fast arc bottom right to top			(use: BOTH_T5_T__BR)
	BOTH_T5__R_BR,	//# Fast arc right to bottom right			(use: BOTH_T5_BR__R)
	BOTH_T5__R_T_,	//# Fast ar right to top				(use: BOTH_T5_T___R)
	BOTH_T5_TR__R,	//# Fast arc top right to right			(use: BOTH_T5__R_TR)
	BOTH_T5_TR_T_,	//# Fast arc top right to top				(use: BOTH_T5_T__TR)
	BOTH_T5_TL__R,	//# Fast arc top left to right			(use: BOTH_T5__R_TL)
	BOTH_T5_TL_TR,	//# Fast arc top left to top right			(use: BOTH_T5_TR_TL)
	BOTH_T5_TL_T_,	//# Fast arc top left to top				(use: BOTH_T5_T__TL)
	BOTH_T5_TL__L,	//# Fast arc top left to left				(use: BOTH_T5__L_TL)
	BOTH_T5__L_TR,	//# Fast arc left to top right			(use: BOTH_T5_TR__L)
	BOTH_T5__L_T_,	//# Fast arc left to top				(use: BOTH_T5_T___L)
	BOTH_T5__L_BL,	//# Fast arc left to bottom left			(use: BOTH_T5_BL__L)
	BOTH_T5_BL_T_,	//# Fast arc bottom left to top			(use: BOTH_T5_T__BL)
	BOTH_T5_BL_TL,	//# Fast arc bottom left to top left		(use: BOTH_T5_TL_BL)
	//Saber Attack Start Transitions
	BOTH_S5_S1_T_,	//# Fast plain transition from stance1 to top-to-bottom Fast weak attack
	BOTH_S5_S1__L,	//# Fast plain transition from stance1 to left-to-right Fast weak attack
	BOTH_S5_S1__R,	//# Fast plain transition from stance1 to right-to-left Fast weak attack
	BOTH_S5_S1_TL,	//# Fast plain transition from stance1 to top-left-to-bottom-right Fast weak attack
	BOTH_S5_S1_BR,	//# Fast plain transition from stance1 to bottom-right-to-top-left Fast weak attack
	BOTH_S5_S1_BL,	//# Fast plain transition from stance1 to bottom-left-to-top-right Fast weak attack
	BOTH_S5_S1_TR,	//# Fast plain transition from stance1 to top-right-to-bottom-left Fast weak attack
	//Saber Attack Return Transitions
	BOTH_R5_B__S1,	//# Fast plain transition from top-to-bottom Fast weak attack to stance1
	BOTH_R5__L_S1,	//# Fast plain transition from left-to-right Fast weak attack to stance1
	BOTH_R5__R_S1,	//# Fast plain transition from right-to-left Fast weak attack to stance1
	BOTH_R5_TL_S1,	//# Fast plain transition from top-left-to-bottom-right Fast weak attack to stance1
	BOTH_R5_BR_S1,	//# Fast plain transition from bottom-right-to-top-left Fast weak attack to stance1
	BOTH_R5_BL_S1,	//# Fast plain transition from bottom-left-to-top-right Fast weak attack to stance1
	BOTH_R5_TR_S1,	//# Fast plain transition from top-right-to-bottom-left Fast weak attack
	//Saber Attack Bounces (first 4 frames of an attack, played backwards)
	BOTH_B5_BR___,	//# Bounce-back if attack from BR is blocked
	BOTH_B5__R___,	//# Bounce-back if attack from R is blocked
	BOTH_B5_TR___,	//# Bounce-back if attack from TR is blocked
	BOTH_B5_T____,	//# Bounce-back if attack from T is blocked
	BOTH_B5_TL___,	//# Bounce-back if attack from TL is blocked
	BOTH_B5__L___,	//# Bounce-back if attack from L is blocked
	BOTH_B5_BL___,	//# Bounce-back if attack from BL is blocked
	//Saber Attack Deflections (last 4 frames of an attack)
	BOTH_D5_BR___,	//# Deflection toward BR
	BOTH_D5__R___,	//# Deflection toward R
	BOTH_D5_TR___,	//# Deflection toward TR
	BOTH_D5_TL___,	//# Deflection toward TL
	BOTH_D5__L___,	//# Deflection toward L
	BOTH_D5_BL___,	//# Deflection toward BL
	BOTH_D5_B____,	//# Deflection toward B
	//Saber parry anims
	BOTH_P1_S1_T_,	//# Block shot/saber top
	BOTH_P1_S1_TR,	//# Block shot/saber top right
	BOTH_P1_S1_TL,	//# Block shot/saber top left
	BOTH_P1_S1_BL,	//# Block shot/saber bottom left
	BOTH_P1_S1_BR,	//# Block shot/saber bottom right
	//Saber knockaway
	BOTH_K1_S1_T_,	//# knockaway saber top
	BOTH_K1_S1_TR,	//# knockaway saber top right
	BOTH_K1_S1_TL,	//# knockaway saber top left
	BOTH_K1_S1_BL,	//# knockaway saber bottom left
	BOTH_K1_S1_B_,	//# knockaway saber bottom
	BOTH_K1_S1_BR,	//# knockaway saber bottom right
	//Saber attack knocked away
	BOTH_V1_BR_S1,	//# BR attack knocked away
	BOTH_V1__R_S1,	//# R attack knocked away
	BOTH_V1_TR_S1,	//# TR attack knocked away
	BOTH_V1_T__S1,	//# T attack knocked away
	BOTH_V1_TL_S1,	//# TL attack knocked away
	BOTH_V1__L_S1,	//# L attack knocked away
	BOTH_V1_BL_S1,	//# BL attack knocked away
	BOTH_V1_B__S1,	//# B attack knocked away
	//Saber parry broken
	BOTH_H1_S1_T_,	//# saber knocked down from top parry
	BOTH_H1_S1_TR,	//# saber knocked down-left from TR parry
	BOTH_H1_S1_TL,	//# saber knocked down-right from TL parry
	BOTH_H1_S1_BL,	//# saber knocked up-right from BL parry
	BOTH_H1_S1_B_,	//# saber knocked up over head from ready?
	BOTH_H1_S1_BR,	//# saber knocked up-left from BR parry
	//Sabers locked anims
	BOTH_BF2RETURN,	//#
	BOTH_BF2BREAK,	//#
	BOTH_BF2LOCK,	//#
	BOTH_BF1RETURN,	//#
	BOTH_BF1BREAK,	//#
	BOTH_BF1LOCK,	//#
	BOTH_CWCIRCLE_R2__R_S1,	//#
	BOTH_CCWCIRCLE_R2__L_S1,	//#
	BOTH_CWCIRCLE_A2__L__R,	//#
	BOTH_CCWCIRCLE_A2__R__L,	//#
	BOTH_CWCIRCLEBREAK,	//#
	BOTH_CCWCIRCLEBREAK,	//#
	BOTH_CWCIRCLELOCK,	//#
	BOTH_CCWCIRCLELOCK,	//#

	BOTH_SABERFAST_STANCE,
	BOTH_SABERSLOW_STANCE,
	BOTH_A2_STABBACK1,		//# Stab saber backward
	BOTH_ATTACK_BACK,		//# Swing around backwards and attack
	BOTH_JUMPFLIPSLASHDOWN1,//#
	BOTH_JUMPFLIPSTABDOWN,//#
	BOTH_FORCELEAP2_T__B_,//#
	BOTH_LUNGE2_B__T_,//#
	BOTH_CROUCHATTACKBACK1,//#

	//# #sep BOTH_ STANDING
	BOTH_STAND1,			//# Standing idle, no weapon, hands down
	BOTH_STAND1IDLE1,		//# Random standing idle
	BOTH_STAND2,			//# Standing idle with a saber
	BOTH_STAND2IDLE1,		//# Random standing idle
	BOTH_STAND2IDLE2,		//# Random standing idle
	BOTH_STAND3,			//# Standing idle with 2-handed weapon
	BOTH_STAND3IDLE1,		//# Random standing idle
	BOTH_STAND4,			//# hands clasp behind back
	BOTH_STAND4IDLE1,		//# Random standing idle
	BOTH_STAND5,			//# standing idle, no weapon, hand down, back straight
	BOTH_STAND5IDLE1,		//# Random standing idle
	BOTH_STAND6,			//# one handed, gun at side, relaxed stand
	BOTH_STAND7,			//# both hands on hips (female)
	BOTH_STAND8,			//# both hands on hips (male)
	BOTH_STAND1TO3,			//# Transition from stand1 to stand3
	BOTH_STAND3TO1,			//# Transition from stand3 to stand1
	BOTH_STAND1TO2,			//# Transition from stand1 to stand2
	BOTH_STAND2TO1,			//# Transition from stand2 to stand1
	BOTH_STAND2TO4,			//# Transition from stand2 to stand4
	BOTH_STAND4TO2,			//# Transition from stand4 to stand2
	BOTH_STANDTOWALK1,		//# Transition from stand1 to walk1
	BOTH_STAND4TOATTACK2,	//# relaxed stand to 1-handed pistol ready
	BOTH_STANDUP1,			//# standing up and stumbling
	BOTH_STANDUP2,			//# Luke standing up from his meditation platform (cin # 37)
	BOTH_STAND5TOSIT3,		//# transition from stand 5 to sit 3
	BOTH_STAND1_REELO,		//# Reelo in his stand1 position (cin #18)
	BOTH_STAND5_REELO,		//# Reelo in his stand5 position (cin #18)
	BOTH_STAND1TOSTAND5,	//# Transition from stand1 to stand5
	BOTH_STAND5TOSTAND1,	//# Transition from stand5 to stand1
	BOTH_STAND5TOAIM,		//# Transition of Kye aiming his gun at Desann (cin #9)
	BOTH_STAND5STARTLEDLOOKLEFT,	//# Kyle turning to watch the bridge drop (cin #9)
	BOTH_STARTLEDLOOKLEFTTOSTAND5,	//# Kyle returning to stand 5 from watching the bridge drop (cin #9)
	BOTH_STAND5TOSTAND8,	//# Transition from stand5 to stand8
	BOTH_STAND7TOSTAND8,	//# Tavion putting hands on back of chair (cin #11)
	BOTH_STAND8TOSTAND5,	//# Transition from stand8 to stand5
	BOTH_STAND5SHIFTWEIGHT,	//# Weightshift from stand5 to side and back to stand5
	BOTH_STAND5SHIFTWEIGHTSTART,	//# From stand5 to side
	BOTH_STAND5SHIFTWEIGHTSTOP,		//# From side to stand5
	BOTH_STAND5TURNLEFTSTART,		//# Start turning left from stand5
	BOTH_STAND5TURNLEFTSTOP,		//# Stop turning left from stand5
	BOTH_STAND5TURNRIGHTSTART,		//# Start turning right from stand5
	BOTH_STAND5TURNRIGHTSTOP,		//# Stop turning right from stand5
	BOTH_STAND5LOOK180LEFTSTART,	//# Start looking over left shoulder (cin #17)
	BOTH_STAND5LOOK180LEFTSTOP,	//# Stop looking over left shoulder (cin #17)

	BOTH_CONSOLE1START,		//# typing at a console
	BOTH_CONSOLE1,			//# typing at a console
	BOTH_CONSOLE1STOP,		//# typing at a console
	BOTH_CONSOLE2START,		//# typing at a console with comm link in hand (cin #5)
	BOTH_CONSOLE2,			//# typing at a console with comm link in hand (cin #5)
	BOTH_CONSOLE2STOP,		//# typing at a console with comm link in hand (cin #5)
	BOTH_CONSOLE2HOLDCOMSTART,	//# lean in to type at console while holding comm link in hand (cin #5)
	BOTH_CONSOLE2HOLDCOMSTOP,	//# lean away after typing at console while holding comm link in hand (cin #5)

	BOTH_GUARD_LOOKAROUND1,	//# Cradling weapon and looking around
	BOTH_GUARD_IDLE1,		//# Cradling weapon and standing
	BOTH_ALERT1,			//# Startled by something while on guard
	BOTH_GESTURE1,			//# Generic gesture, non-specific
	BOTH_GESTURE2,			//# Generic gesture, non-specific
	BOTH_GESTURE3,			//# Generic gesture, non-specific
	BOTH_WALK1TALKCOMM1,	//# Talking into coom link while walking
	BOTH_TALK1,				//# Generic talk anim
	BOTH_TALK2,				//# Generic talk anim
	BOTH_TALKCOMM1START,	//# Start talking into a comm link
	BOTH_TALKCOMM1,			//# Talking into a comm link
	BOTH_TALKCOMM1STOP,		//# Stop talking into a comm link
	BOTH_TALKGESTURE1,		//# Generic talk anim
	BOTH_TALKGESTURE2,		//# Generic talk anim
	BOTH_TALKGESTURE3,		//# Generic talk anim
	BOTH_TALKGESTURE4START,	//# Beginning talk anim 4
	BOTH_TALKGESTURE4,		//# Talk gesture 4
	BOTH_TALKGESTURE4STOP,	//# Ending talk anim 4
	BOTH_TALKGESTURE5START,	//# Start hand on chin
	BOTH_TALKGESTURE5,		//# Hand on chin
	BOTH_TALKGESTURE5STOP,	//# Stop hand on chin
	BOTH_TALKGESTURE6START,	//# Starting Motions to self
	BOTH_TALKGESTURE6,		//# Pointing at self
	BOTH_TALKGESTURE6STOP,	//# Ending Motions to self
	BOTH_TALKGESTURE7START,	//# Start touches Kyle on shoulder
	BOTH_TALKGESTURE7,		//# Hold touches Kyle on shoulder
	BOTH_TALKGESTURE7STOP,	//# Ending touches Kyle on shoulder
	BOTH_TALKGESTURE8START,	//# Lando's chin hold
	BOTH_TALKGESTURE8,			//# Lando's chin hold
	BOTH_TALKGESTURE8STOP,	//# Lando's chin hold
	BOTH_TALKGESTURE9,		//# Same as gesture 2 but with the right hand
	BOTH_TALKGESTURE10,		//# Shoulder shrug
	BOTH_TALKGESTURE11START,	//# Arms folded across chest
	BOTH_TALKGESTURE11STOP,	//# Arms folded across chest
	BOTH_TALKGESTURE12,		//# Tavion taunting Kyle
	BOTH_TALKGESTURE13START,	//# Luke warning Kyle
	BOTH_TALKGESTURE13,			//# Luke warning Kyle
	BOTH_TALKGESTURE13STOP,	//# Luke warning Kyle
	BOTH_TALKGESTURE14,			//# Luke gesturing to Kyle
	BOTH_TALKGESTURE15START,	//# Desann taunting Kyle
	BOTH_TALKGESTURE15,			//# Desann taunting Kyle
	BOTH_TALKGESTURE15STOP,		//# Desann taunting Kyle
	BOTH_TALKGESTURE16,			//# Bartender gesture cin #15
	BOTH_TALKGESTURE17,			//# Bartender gesture cin #15
	BOTH_TALKGESTURE18,			//# Bartender gesture cin #15
	BOTH_TALKGESTURE19START,	//# Desann lifting his arm "Join me" (cin #34)
	BOTH_TALKGESTURE19STOP,		//# Desann lifting his arm "Join me" (cin #34)
	BOTH_TALKGESTURE20START,	//# Kyle lifting his arm "Join us" (cin #34)
	BOTH_TALKGESTURE21,			//# generic talk gesture from stand3
	BOTH_TALKGESTURE22,			//# generic talk gesture from stand3
	BOTH_TALKGESTURE23,			//# generic talk gesture from stand3
	BOTH_PAUSE1START,			//# Luke pauses to warn Kyle (cin #24) start
	BOTH_PAUSE1STOP,			//# Luke pauses to warn Kyle (cin #24) stop

	BOTH_HEADTILTLSTART,		//# Head tilt to left
	BOTH_HEADTILTLSTOP,			//# Head tilt to left
	BOTH_HEADTILTRSTART,		//# Head tilt to right
	BOTH_HEADTILTRSTOP,			//# Head tilt to right
	BOTH_HEADNOD,				//# Head shake YES
	BOTH_HEADSHAKE,				//# Head shake NO
	BOTH_HEADSHAKE1_REELO,		//# Head shake NO for Reelo
	BOTH_SITHEADTILTLSTART,		//# Head tilt to left from seated position
	BOTH_SITHEADTILTLSTOP,		//# Head tilt to left from seated position
	BOTH_SITHEADTILTRSTART,		//# Head tilt to right from seated position
	BOTH_SITHEADTILTRSTOP,		//# Head tilt to right from seated position
	BOTH_SITHEADNOD,			//# Head shake YES from seated position
	BOTH_SITHEADSHAKE,			//# Head shake NO from seated position
	BOTH_SIT2HEADTILTLSTART,	//# Head tilt to left from seated position 2
	BOTH_SIT2HEADTILTLSTOP,		//# Head tilt to left from seated position 2

	BOTH_REACH1START,		//# Monmothma reaching for crystal
	BOTH_REACH1STOP,		//# Monmothma reaching for crystal

	BOTH_EXAMINE1START,		//# Start Mon Mothma examining crystal
	BOTH_EXAMINE1,			//# Mon Mothma examining crystal
	BOTH_EXAMINE1STOP,		//# Stop Mon Mothma examining crystal
	BOTH_EXAMINE2START,	//# Start Kyle tossing crystal
	BOTH_EXAMINE2,			//# Hold Kyle tossing crystal
	BOTH_EXAMINE2STOP,		//# End Kyle tossing crystal
	BOTH_EXAMINE3START,	//# Start Lando looking around corner
	BOTH_EXAMINE3,			//# Hold Lando looking around corner
	BOTH_EXAMINE3STOP,		//# End Lando looking around corner

	BOTH_LEANLEFT2START,	//# Start leaning left in chair
	BOTH_LEANLEFT2STOP,		//# Stop leaning left in chair
	BOTH_LEANRIGHT3START,	//# Start Lando leaning on wall
	BOTH_LEANRIGHT3,			//# Lando leaning on wall
	BOTH_LEANRIGHT3STOP,		//# Stop Lando leaning on wall

	BOTH_FORCEFOUNTAIN1_START,	//# Kyle being lifted into the Force Fountain (cin #10)
	BOTH_FORCEFOUNTAIN1_MIDDLE,	//# Kyle changing to looping position in the Force Fountain (cin #10)
	BOTH_FORCEFOUNTAIN1_LOOP,	//# Kyle being spun in the Force Fountain (cin #10)
	BOTH_FORCEFOUNTAIN1_STOP,	//# Kyle being set down out of the Force Fountain (cin #10)
	BOTH_THUMBING1,				//# Lando gesturing with thumb over his shoulder (cin #19)
	BOTH_COME_ON1,				//# Jan gesturing to Kyle (cin #32a)
	BOTH_STEADYSELF1,			//# Jan trying to keep footing (cin #32a)
	BOTH_STEADYSELF1END,		//# Return hands to side from STEADSELF1 Kyle (cin#5)
	BOTH_SILENCEGESTURE1,		//# Luke silencing Kyle with a raised hand (cin #37)
	BOTH_REACHFORSABER1,		//# Luke holding hand out for Kyle's saber (cin #37)
	BOTH_PUNCHER1,				//# Jan punching Kyle in the shoulder (cin #37)
	BOTH_CONSTRAINER1HOLD,		//# Static pose of starting Tavion constraining Jan (cin #9)
	BOTH_CONSTRAINEE1HOLD,		//# Static pose of starting Jan being constrained by Tavion (cin #9)
	BOTH_CONSTRAINER1STAND,		//# Tavion constraining Jan in a stand pose (cin #9)
	BOTH_CONSTRAINEE1STAND,		//# Jan being constrained in a stand pose (cin #9)
	BOTH_CONSTRAINER1WALK,		//# Tavion shoving jan forward (cin #9)
	BOTH_CONSTRAINEE1WALK,		//# Jan being shoved forward by Tavion (cin #9)
	BOTH_CONSTRAINER1LOOP,		//# Tavion walking with Jan in a loop (cin #9)
	BOTH_CONSTRAINEE1LOOP,		//# Jan walking with Tavion in a loop (cin #9)
	BOTH_SABERKILLER1,			//# Tavion about to strike Jan with saber (cin #9)
	BOTH_SABERKILLEE1,			//# Jan about to be struck by Tavion with saber (cin #9)
	BOTH_HANDSHAKER1START,		//# Luke shaking Kyle's hand (cin #37)
	BOTH_HANDSHAKER1LOOP,		//# Luke shaking Kyle's hand (cin #37)
	BOTH_HANDSHAKEE1START,		//# Kyle shaking Luke's hand (cin #37)
	BOTH_HANDSHAKEE1LOOP,		//# Kyle shaking Luke's hand (cin #37)
	BOTH_LAUGH1START,			//# Reelo leaning forward before laughing (cin #18)
	BOTH_LAUGH1STOP,			//# Reelo laughing (cin #18)
	BOTH_ESCAPEPOD_LEAVE1,	//# Kyle leaving escape pod (cin #33)
	BOTH_ESCAPEPOD_LEAVE2,	//# Jan leaving escape pod (cin #33)
	BOTH_HUGGER1,			//# Kyle hugging Jan (cin #29)
	BOTH_HUGGERSTOP1,		//# Kyle stop hugging Jan but don't let her go (cin #29)
	BOTH_HUGGERSTOP2,		//# Kyle let go of Jan and step back (cin #29)
	BOTH_HUGGEE1,			//# Jan being hugged (cin #29)
	BOTH_HUGGEESTOP1,		//# Jan stop being hugged but don't let go (cin #29)
	BOTH_HUGGEESTOP2,		//# Jan released from hug (cin #29)
	BOTH_KISSER1,			//# Temp until the Kiss anim gets split up
	BOTH_KISSER1START1,		//# Kyle start kissing Jan
	BOTH_KISSER1START2,		//# Kyle start kissing Jan
	BOTH_KISSER1LOOP,		//# Kyle loop kissing Jan
	BOTH_KISSER1STOP,		//# Temp until the Kiss anim gets split up
	BOTH_KISSER1STOP1,		//# Kyle stop kissing but don't let go
	BOTH_KISSER1STOP2,		//# Kyle step back from Jan
	BOTH_KISSEE1,			//# Temp until the Kiss anim gets split up
	BOTH_KISSEE1START1,		//# Jan start being kissed
	BOTH_KISSEE1START2,		//# Jan start being kissed
	BOTH_KISSEE1LOOP,		//# Jan loop being kissed
	BOTH_KISSEE1STOP,		//# Temp until the Kiss anim gets split up
	BOTH_KISSEE1STOP1,		//# Jan stop being kissed but don't let go
	BOTH_KISSEE1STOP2,		//# Jan wait for Kyle to step back
	BOTH_BARTENDER_IDLE1,	//# Bartender idle in cin #15
	BOTH_BARTENDER_THROW1,	//# Bartender throws glass in cin #15
	BOTH_BARTENDER_COWERSTART,	//# Start of Bartender raising both hands up in surrender (cin #16)
	BOTH_BARTENDER_COWERLOOP,	//# Loop of Bartender waving both hands in surrender (cin #16)
	BOTH_BARTENDER_COWER,		//# Single frame of Bartender waving both hands in surrender (cin #16)
	BOTH_THREATEN1_START,	//# First frame of Kyle threatening Bartender with lightsaber (cin #16)
	BOTH_THREATEN1,			//# Kyle threatening Bartender with lightsaber (cin #16)
	BOTH_RADIO_ONOFF,		//# Mech Galak turning on his suit radio (cin #32)
	BOTH_TRIUMPHANT1START,	//# Mech Galak raising his arms in victory (cin #32)
	BOTH_TRIUMPHANT1STARTGESTURE,	//# Mech Galak raising his arms in victory (cin #32)
	BOTH_TRIUMPHANT1STOP,	//# Mech Galak lowering his arms in victory (cin #32)

	BOTH_SABERTHROW1START,		//# Desann throwing his light saber (cin #26)
	BOTH_SABERTHROW1STOP,		//# Desann throwing his light saber (cin #26)
	BOTH_SABERTHROW2START,		//# Kyle throwing his light saber (cin #32)
	BOTH_SABERTHROW2STOP,		//# Kyle throwing his light saber (cin #32)

	BOTH_COVERUP1_LOOP,		//# animation of getting in line of friendly fire
	BOTH_COVERUP1_START,	//# transitions from stand to coverup1_loop
	BOTH_COVERUP1_END,		//# transitions from coverup1_loop to stand

	BOTH_INJURED4,			//# Injured pose 4
	BOTH_INJURED4TO5,		//# Transition from INJURED4 to INJURED5
	BOTH_INJURED5,			//# Injured pose 5

	//# #sep BOTH_ SITTING/CROUCHING
	BOTH_SIT1STAND,			//# Stand up from First sitting anim
	BOTH_SIT1,				//# Normal chair sit.
	BOTH_SIT2,				//# Lotus position.
	BOTH_SIT3,				//# Sitting in tired position, elbows on knees

	BOTH_SIT2TO3,			//# Trans from sit2 to sit3?
	BOTH_SIT2TOSTAND5,		//# Transition from sit 2 to stand 5
	BOTH_STAND5TOSIT2,		//# Transition from stand 5 to sit 2
	BOTH_SIT2TOSIT4,		//# Trans from sit2 to sit4 (cin #12) Luke leaning back from lotus position.
	BOTH_SIT3TO1,			//# Trans from sit3 to sit1?
	BOTH_SIT3TO2,			//# Trans from sit3 to sit2?
	BOTH_SIT3TOSTAND5,		//# transition from sit 3 to stand 5

	BOTH_SIT4TO5,			//# Trans from sit4 to sit5
	BOTH_SIT4TO6,			//# Trans from sit4 to sit6
	BOTH_SIT5TO4,			//# Trans from sit5 to sit4
	BOTH_SIT5TO6,			//# Trans from sit5 to sit6
	BOTH_SIT6TO4,			//# Trans from sit6 to sit4
	BOTH_SIT6TO5,			//# Trans from sit6 to sit5
	BOTH_SIT7,				//# sitting with arms over knees, no weapon
	BOTH_SIT7TOSTAND1,		//# getting up from sit7 into stand1

	BOTH_CROUCH1,			//# Transition from standing to crouch
	BOTH_CROUCH1IDLE,		//# Crouching idle
	BOTH_CROUCH1WALK,		//# Walking while crouched
	BOTH_CROUCH1WALKBACK,	//# Walking while crouched
	BOTH_UNCROUCH1,			//# Transition from crouch to standing
	BOTH_CROUCH2IDLE,		//# crouch and resting on back righ heel, no weapon
	BOTH_CROUCH2TOSTAND1,	//# going from crouch2 to stand1
	BOTH_CROUCH3,			//# Desann crouching down to Kyle (cin 9)
	BOTH_UNCROUCH3,			//# Desann uncrouching down to Kyle (cin 9)
	BOTH_CROUCH4,			//# Slower version of crouch1 for cinematics
	BOTH_UNCROUCH4,			//# Slower version of uncrouch1 for cinematics
	BOTH_GET_UP1,			//# Get up from the ground, face down
	BOTH_GET_UP2,			//# Get up from the ground, face up

	BOTH_COCKPIT_SIT,			//# sit in a cockpit.

	BOTH_GUNSIT1,			//# sitting on an emplaced gun.

	BOTH_DEATH14_UNGRIP,	//# Desann's end death (cin #35)
	BOTH_DEATH14_SITUP,		//# Tavion sitting up after having been thrown (cin #23)
	BOTH_KNEES1,			//# Tavion on her knees
	BOTH_KNEES2,			//# Tavion on her knees looking down
	BOTH_KNEES2TO1,			//# Transition of KNEES2 to KNEES1
	BOTH_RUMMAGE1START,	//# Kyle rummaging for crystal (cin 2)
	BOTH_RUMMAGE1,			//# Kyle rummaging for crystal (cin 2)
	BOTH_RUMMAGE1STOP,		//# Kyle rummaging for crystal (cin 2)
	BOTH_HOLDGLASS1,		//# Bartender holds glass (cin# 14)
	BOTH_SLIDEGLASS1,		//# Bartender slides glass (cin# 14)
	BOTH_SLAMSABERDOWN,		//# Kyle slamming his saber on the bar top (cin# 14)

	//# #sep BOTH_ MOVING
	BOTH_WALK1,				//# Normal walk
	BOTH_WALK2,				//# Normal walk
	BOTH_WALK3,				//# Goes with stand3
	BOTH_WALK4,				//# Walk cycle goes to a stand4
	BOTH_WALK5,				//# Tavion taunting Kyle (cin 22)
	BOTH_WALK6,				//# Slow walk for Luke (cin 12)
	BOTH_WALK7,				//# Fast walk
	BOTH_WALK8,				//# Normal walk with hands behind back (Luke in cin#12)
	BOTH_WALK9,				//# Lando walk (cin #17)
	BOTH_WALK10,			//# Lando walk (cin #17)
	BOTH_WALKTORUN1,		//# transition from walk to run
	BOTH_RUN1,				//# Full run
	BOTH_RUN1START,			//# Start into full run1
	BOTH_RUN1STOP,			//# Stop from full run1
	BOTH_RUN2,				//# Full run
	BOTH_RUNINJURED1,		//# Run with injured left leg
	BOTH_STRAFE_LEFT1,		//# Sidestep left, should loop
	BOTH_STRAFE_RIGHT1,		//# Sidestep right, should loop
	BOTH_RUNSTRAFE_LEFT1,	//# Sidestep left, should loop
	BOTH_RUNSTRAFE_RIGHT1,	//# Sidestep right, should loop
	BOTH_TURN_LEFT1,		//# Turn left, should loop
	BOTH_TURN_RIGHT1,		//# Turn right, should loop
	BOTH_TURNSTAND1,		//# Turn from STAND1 position
	BOTH_TURNSTAND2,		//# Turn from STAND2 position
	BOTH_TURNSTAND3,		//# Turn from STAND3 position
	BOTH_TURNSTAND4,		//# Turn from STAND4 position
	BOTH_TURNSTAND5,		//# Turn from STAND5 position
	BOTH_TURNCROUCH1,		//# Turn from CROUCH1 position
	BOTH_RUNAWAY1,			//# Running scared
	BOTH_SWIM1,				//# Swimming

	BOTH_WALKBACK1,			//# Walk1 backwards
	BOTH_WALKBACK2,			//# Walk2 backwards
	BOTH_RUNBACK1,			//# Run1 backwards
	BOTH_RUNBACK2,			//# Run1 backwards

	//# #sep BOTH_ JUMPING
	BOTH_JUMP1,				//# Jump - wind-up and leave ground
	BOTH_INAIR1,			//# In air loop (from jump)
	BOTH_LAND1,				//# Landing (from in air loop)
	BOTH_LAND2,				//# Landing Hard (from a great height)

	BOTH_JUMPBACK1,			//# Jump backwards - wind-up and leave ground
	BOTH_INAIRBACK1,		//# In air loop (from jump back)
	BOTH_LANDBACK1,			//# Landing backwards(from in air loop)

	BOTH_JUMPLEFT1,			//# Jump left - wind-up and leave ground
	BOTH_INAIRLEFT1,		//# In air loop (from jump left)
	BOTH_LANDLEFT1,			//# Landing left(from in air loop)

	BOTH_JUMPRIGHT1,		//# Jump right - wind-up and leave ground
	BOTH_INAIRRIGHT1,		//# In air loop (from jump right)
	BOTH_LANDRIGHT1,		//# Landing right(from in air loop)

	BOTH_FORCEJUMP1,		//# Jump - wind-up and leave ground
	BOTH_FORCEINAIR1,		//# In air loop (from jump)
	BOTH_FORCELAND1,		//# Landing (from in air loop)

	BOTH_FORCEJUMPBACK1,	//# Jump backwards - wind-up and leave ground
	BOTH_FORCEINAIRBACK1,	//# In air loop (from jump back)
	BOTH_FORCELANDBACK1,	//# Landing backwards(from in air loop)

	BOTH_FORCEJUMPLEFT1,	//# Jump left - wind-up and leave ground
	BOTH_FORCEINAIRLEFT1,	//# In air loop (from jump left)
	BOTH_FORCELANDLEFT1,	//# Landing left(from in air loop)

	BOTH_FORCEJUMPRIGHT1,	//# Jump right - wind-up and leave ground
	BOTH_FORCEINAIRRIGHT1,	//# In air loop (from jump right)
	BOTH_FORCELANDRIGHT1,	//# Landing right(from in air loop)
	//# #sep BOTH_ ACROBATICS
	BOTH_FLIP_F,			//# Flip forward
	BOTH_FLIP_B,			//# Flip backwards
	BOTH_FLIP_L,			//# Flip left
	BOTH_FLIP_R,			//# Flip right

	BOTH_ROLL_F,			//# Roll forward
	BOTH_ROLL_B,			//# Roll backward
	BOTH_ROLL_L,			//# Roll left
	BOTH_ROLL_R,			//# Roll right
	BOTH_ROLL_FR,			//# Roll forward right
	BOTH_ROLL_FL,			//# Roll forward left
	BOTH_ROLL_BR,			//# Roll back right
	BOTH_ROLL_BL,			//# Roll back left

	BOTH_HOP_F,				//# quickstep forward
	BOTH_HOP_B,				//# quickstep backwards
	BOTH_HOP_L,				//# quickstep left
	BOTH_HOP_R,				//# quickstep right

	BOTH_DODGE_FL,			//# lean-dodge forward left
	BOTH_DODGE_FR,			//# lean-dodge forward right
	BOTH_DODGE_BL,			//# lean-dodge backwards left
	BOTH_DODGE_BR,			//# lean-dodge backwards right
	BOTH_DODGE_L,			//# lean-dodge left
	BOTH_DODGE_R,			//# lean-dodge right

	BOTH_DIVE1,				//# Dive!

	BOTH_ENGAGETAUNT,
	BOTH_ARIAL_LEFT,		//#
	BOTH_ARIAL_RIGHT,		//#
	BOTH_CARTWHEEL_LEFT,	//#
	BOTH_CARTWHEEL_RIGHT,	//#
	BOTH_FLIP_LEFT,			//#
	BOTH_FLIP_BACK1,		//#
	BOTH_FLIP_BACK2,		//#
	BOTH_FLIP_BACK3,		//#
	BOTH_BUTTERFLY_LEFT,	//#
	BOTH_BUTTERFLY_RIGHT,	//#
	BOTH_WALL_RUN_RIGHT,	//#
	BOTH_WALL_RUN_RIGHT_FLIP,//#
	BOTH_WALL_RUN_RIGHT_STOP,//#
	BOTH_WALL_RUN_LEFT,		//#
	BOTH_WALL_RUN_LEFT_FLIP,//#
	BOTH_WALL_RUN_LEFT_STOP,//#
	BOTH_WALL_FLIP_RIGHT,	//#
	BOTH_WALL_FLIP_LEFT,	//#
	BOTH_WALL_FLIP_FWD,		//#
	BOTH_KNOCKDOWN1,		//# knocked backwards
	BOTH_KNOCKDOWN2,		//# knocked backwards hard
	BOTH_KNOCKDOWN3,		//#	knocked forwards
	BOTH_KNOCKDOWN4,		//# knocked backwards from crouch
	BOTH_KNOCKDOWN5,		//# dupe of 3 - will be removed
	BOTH_GETUP1,			//#
	BOTH_GETUP2,			//#
	BOTH_GETUP3,			//#
	BOTH_GETUP4,			//#
	BOTH_GETUP5,			//#
	BOTH_GETUP_CROUCH_F1,	//#
	BOTH_GETUP_CROUCH_B1,	//#
	BOTH_FORCE_GETUP_F1,	//#
	BOTH_FORCE_GETUP_F2,	//#
	BOTH_FORCE_GETUP_B1,	//#
	BOTH_FORCE_GETUP_B2,	//#
	BOTH_FORCE_GETUP_B3,	//#
	BOTH_FORCE_GETUP_B4,	//#
	BOTH_FORCE_GETUP_B5,	//#
	BOTH_FORCE_GETUP_B6,	//#
	BOTH_WALL_FLIP_BACK1,	//#
	BOTH_WALL_FLIP_BACK2,	//#
	BOTH_SPIN1,				//#
	BOTH_CEILING_CLING,		//# clinging to ceiling
	BOTH_CEILING_DROP,		//# dropping from ceiling cling

	//TESTING
	BOTH_FJSS_TR_BL,		//# jump spin slash tr to bl
	BOTH_FJSS_TL_BR,		//# jump spin slash bl to tr
	BOTH_DEATHFROMBACKSLASH,//#
	BOTH_RIGHTHANDCHOPPEDOFF,//#
	BOTH_DEFLECTSLASH__R__L_FIN,//#
	BOTH_BASHED1,//#
	BOTH_ARIAL_F1,//#
	BOTH_BUTTERFLY_FR1,//#
	BOTH_BUTTERFLY_FL1,//#

	//# #sep BOTH_ MISC MOVEMENT
	BOTH_HIT1,				//# Kyle hit by crate in cin #9
	BOTH_LADDER_UP1,		//# Climbing up a ladder with rungs at 16 unit intervals
	BOTH_LADDER_DWN1,		//# Climbing down a ladder with rungs at 16 unit intervals
	BOTH_LADDER_IDLE,		//#	Just sitting on the ladder
	BOTH_ONLADDER_BOT1,		//# Getting on the ladder at the bottom
	BOTH_OFFLADDER_BOT1,	//# Getting off the ladder at the bottom
	BOTH_ONLADDER_TOP1,		//# Getting on the ladder at the top
	BOTH_OFFLADDER_TOP1,	//# Getting off the ladder at the top
	BOTH_LIFT1,				//# Lifting someone/thing over their shoulder
	BOTH_STEP1,				//# telsia checking out lake cinematic9.2
	BOTH_HITWALL1,			//# cin.18, Kenn hit by borg into wall 56 units away
	BOTH_AMBUSHLAND1,		//# landing from fall on victim
	BOTH_BIRTH1,			//# birth from jumping through walls

	//# #sep BOTH_ FLYING IDLE
	BOTH_FLY_IDLE1,			//# Flying Idle 1
	BOTH_FLY_IDLE2,			//# Flying Idle 2
	BOTH_FLY_SHIELDED,		//# For sentry droid, shields in


	//# #sep BOTH_ FLYING MOVING
	BOTH_FLY_START1,		//# Start flying
	BOTH_FLY_STOP1,			//# Stop flying
	BOTH_FLY_LOOP1,			//# Normal flying, should loop
	BOTH_FLOAT1,			//# Crew floating through space 1
	BOTH_FLOAT2,			//# Crew floating through space 2
	BOTH_FLOATCONSOLE1,		//# Crew floating and working on console

	//# #sep BOTH_ SWIMMING
	BOTH_SWIM_IDLE1,		//# Swimming Idle 1
	BOTH_SWIMFORWARD,		//# Swim forward loop

	//# #sep BOTH_ LYING
	BOTH_LIE_DOWN1,			//# From a stand position, get down on ground, face down
	BOTH_LIE_DOWN2,			//# From a stand position, get down on ground, face up
	BOTH_LIE_DOWN3,			//# reaction to local disnode being destroyed
	BOTH_PAIN2WRITHE1,		//# Transition from upright position to writhing on ground anim
	BOTH_PRONE2RLEG,		//# Lying on ground reach to grab right leg
	BOTH_PRONE2LLEG,		//# Lying on ground reach to grab left leg
	BOTH_WRITHING1,			//# Lying on ground on back writhing in pain
	BOTH_WRITHING1RLEG,		//# Lying on ground writhing in pain, holding right leg
	BOTH_WRITHING1LLEG,		//# Lying on ground writhing in pain, holding left leg
	BOTH_WRITHING2,			//# Lying on ground on front writhing in pain
	BOTH_INJURED1,			//# Lying down, against wall - can also be sleeping against wall
	BOTH_INJURED2,			//# Injured pose 2
	BOTH_INJURED3,			//# Injured pose 3
	BOTH_INJURED6,			//# Injured pose 6
	BOTH_INJURED6ATTACKSTART,	//# Start attack while in injured 6 pose
	BOTH_INJURED6ATTACKSTOP,	//# End attack while in injured 6 pose
	BOTH_INJURED6COMBADGE,	//# Hit combadge while in injured 6 pose
	BOTH_INJURED6POINT,		//# Chang points to door while in injured state
	BOTH_INJUREDTOSTAND1,	//# Runinjured to stand1

	BOTH_PROPUP1,			//# Kyle getting up from having been knocked down (cin #9 end)
	BOTH_CRAWLBACK1,		//# Lying on back, crawling backwards with elbows
	BOTH_SITWALL1,			//# Sitting against a wall
	BOTH_SLEEP1,			//# laying on back-rknee up-rhand on torso
	BOTH_SLEEP2,			//# on floor-back against wall-arms crossed
	BOTH_SLEEP3,			//# Sleeping in a chair
	BOTH_SLEEP4,			//# Sleeping slumped over table
	BOTH_SLEEP5,			//# Laying on side sleeping on flat sufrace
	BOTH_SLEEP6START,		//# Kyle leaning back to sleep (cin 20)
	BOTH_SLEEP6STOP,		//# Kyle waking up and shaking his head (cin 21)
	BOTH_SLEEP1GETUP,		//# alarmed and getting up out of sleep1 pose to stand
	BOTH_SLEEP1GETUP2,		//#
	BOTH_SLEEP2GETUP,		//# alarmed and getting up out of sleep2 pose to stand
	BOTH_SLEEP3GETUP,		//# alarmed and getting up out of sleep3 pose to stand
	BOTH_SLEEP3DEATH,		//# death in chair, from sleep3 idle
	BOTH_SLEEP3DEAD,		//# death in chair, from sleep3 idle

	BOTH_SLEEP_IDLE1,		//# rub face and nose while asleep from sleep pose 1
	BOTH_SLEEP_IDLE2,		//# shift position while asleep - stays in sleep2
	BOTH_SLEEP_IDLE3,		//# Idle anim from sleep pose 3
	BOTH_SLEEP_IDLE4,		//# Idle anim from sleep pose 4
	BOTH_SLEEP1_NOSE,		//# Scratch nose from SLEEP1 pose
	BOTH_SLEEP2_SHIFT,		//# Shift in sleep from SLEEP2 pose
	BOTH_RESTRAINED1,		//# Telsia tied to medical table
	BOTH_RESTRAINED1POINT,	//# Telsia tied to medical table pointing at Munro
	BOTH_LIFTED1,			//# Fits with BOTH_LIFT1, lifted on shoulder
	BOTH_CARRIED1,			//# Fits with TORSO_CARRY1, carried over shoulder
	BOTH_CARRIED2,			//# Laying over object

	BOTH_CHOKE1START,		//# tavion in force grip choke
	BOTH_CHOKE1STARTHOLD,	//# loop of tavion in force grip choke
	BOTH_CHOKE1,			//# tavion in force grip choke

	BOTH_CHOKE2,			//# tavion recovering from force grip choke
	BOTH_CHOKE3,			//# left-handed choke (for people still holding a weapon)

	//# #sep BOTH_ HUNTER-SEEKER BOT-SPECIFIC
	BOTH_POWERUP1,			//# Wakes up

	BOTH_TURNON,			//# Protocol Droid wakes up
	BOTH_TURNOFF,			//# Protocol Droid shuts off

	BOTH_BUTTON1,			//# Single button push with right hand
	BOTH_BUTTON2,			//# Single button push with left finger
	BOTH_BUTTON_HOLD,		//# Single button hold with left hand
	BOTH_BUTTON_RELEASE,	//# Single button release with left hand

	//# JEDI-SPECIFIC
	BOTH_RESISTPUSH,		//# plant yourself to resist force push/pulls.
	BOTH_FORCEPUSH,			//# Use off-hand to do force power.
	BOTH_FORCEPULL,			//# Use off-hand to do force power.
	BOTH_MINDTRICK1,			//# Use off-hand to do mind trick
	BOTH_MINDTRICK2,			//# Use off-hand to do distraction
	BOTH_FORCELIGHTNING,		//# Use off-hand to do lightning
	BOTH_FORCELIGHTNING_START,	//# Use off-hand to do lightning - start
	BOTH_FORCELIGHTNING_HOLD,	//# Use off-hand to do lightning - hold
	BOTH_FORCELIGHTNING_RELEASE,//# Use off-hand to do lightning - release
	BOTH_FORCEHEAL_START,		//# Healing meditation pose start
	BOTH_FORCEHEAL_STOP,		//# Healing meditation pose end
	BOTH_FORCEHEAL_QUICK,		//# Healing meditation gesture
	BOTH_SABERPULL,			//# Use off-hand to do force power.
	BOTH_FORCEGRIP1,		//# force-gripping (no anim?)
	BOTH_FORCEGRIP3,		//# force-gripping (right hand)
	BOTH_FORCEGRIP3THROW,	//# throwing while force-gripping (right hand)
	BOTH_FORCEGRIP_HOLD,	//# Use off-hand to do grip - hold
	BOTH_FORCEGRIP_RELEASE,//# Use off-hand to do grip - release
	BOTH_TOSS1,				//# throwing to left after force gripping
	BOTH_TOSS2,				//# throwing to right after force gripping

	BOTH_COCKPIT_TALKR1START,		//# turn head from straight forward to looking full right
	BOTH_COCKPIT_TALKR1STARTTOMID,	//# from TALKR1START to looking at hologram (cin #1)
	BOTH_COCKPIT_TALKR1MIDTOSTART,	//# from looking at hologram to TALKR1START (cin #1)
	BOTH_COCKPIT_TALKR1STOP,		//# return head to straight forward from BOTH_COCKPIT_TALKR1
	BOTH_COCKPIT_TALKR1STOPTOMID,	//# from TALKR1STOP to TALKR1MID
	BOTH_COCKPIT_TALKR1MIDTOSTOP,	//# from looking at hologram to TALKR1STOP (cin #1)
	BOTH_COCKPIT_TALKR1,			//# talk to right side

	BOTH_COCKPIT_TALKL1START,		//# turn head from straight forward to looking full left
	BOTH_COCKPIT_TALKL1STARTTOMID,	//# from TALKL1START to looking at hologram (cin #1)
	BOTH_COCKPIT_TALKL1MIDTOSTART,	//# from looking at hologram to TALKL1START (cin #1)
	BOTH_COCKPIT_TALKL1STOP,		//# return head to straight forward from BOTH_COCKPIT_TALKL1
	BOTH_COCKPIT_TALKL1STOPTOMID,	//# from TALKL1STOP to TALKL1MID
	BOTH_COCKPIT_TALKL1MIDTOSTOP,	//# from looking at hologram to TALKL1STOP (cin #1)
	BOTH_COCKPIT_TALKL1,			//# talk to left side

	BOTH_COCKPIT_CONSOLE1,			//# type at controls
	BOTH_COCKPIT_CONSOLE2,			//# type at controls
	BOTH_COCKPIT_CONSOLE2_PARTIAL,	//# last part of console2 anim (cin #1) used by Jan

	BOTH_COCKPIT_HEADNOD,			//# nod head yes while sitting
	BOTH_COCKPIT_HEADSHAKE,			//# shake head no while sitting

	BOTH_COCKPIT_HEADTILTLSTART,	//# start tilt head left while sitting
	BOTH_COCKPIT_HEADTILTLSTOP,		//# stop tilt head left while sitting
	BOTH_COCKPIT_HEADTILTRSTART,	//# start tilt head right while sitting
	BOTH_COCKPIT_HEADTILTRSTOP,		//# stop tilt head right while sitting

	BOTH_COCKPIT_TALKGESTURE7START,		//# Lando's supporting hand to Kyle (cin #21)
	BOTH_COCKPIT_TALKGESTURE7STOP,		//# Lando's supporting hand away from Kyle (cin #21)
	BOTH_COCKPIT_TALKGESTURE8START,		//# Hand to Lando's chin (cin #21)
	BOTH_COCKPIT_TALKGESTURE8STOP,		//# hand away from Lando's chin *cin #21)
	BOTH_COCKPIT_TALKGESTURE11START,	//#
	BOTH_COCKPIT_TALKGESTURE11STOP,		//#

	BOTH_COCKPIT_SLEEP6START,		//#
	BOTH_COCKPIT_SLEEP6STOP,		//#

	//=================================================
	//ANIMS IN WHICH ONLY THE UPPER OBJECTS ARE IN MD3
	//=================================================
	//# #sep TORSO_ WEAPON-RELATED
	TORSO_DROPWEAP1,		//# Put weapon away
	TORSO_DROPWEAP2,		//# Put weapon away
	TORSO_DROPWEAP3,		//# Put weapon away
	TORSO_DROPWEAP4,		//# Put weapon away
	TORSO_RAISEWEAP1,		//# Draw Weapon
	TORSO_RAISEWEAP2,		//# Draw Weapon
	TORSO_RAISEWEAP3,		//# Draw Weapon
	TORSO_RAISEWEAP4,		//# Draw Weapon
	TORSO_WEAPONREADY1,		//# Ready to fire stun baton
	TORSO_WEAPONREADY2,		//# Ready to fire one-handed blaster pistol
	TORSO_WEAPONREADY3,		//# Ready to fire blaster rifle
	TORSO_WEAPONREADY4,		//# Ready to fire sniper rifle
	TORSO_WEAPONREADY5,		//# Ready to fire bowcaster
	TORSO_WEAPONREADY6,		//# Ready to fire ???
	TORSO_WEAPONREADY7,		//# Ready to fire ???
	TORSO_WEAPONREADY8,		//# Ready to fire ???
	TORSO_WEAPONREADY9,		//# Ready to fire rocket launcher
	TORSO_WEAPONREADY10,	//# Ready to fire thermal det
	TORSO_WEAPONREADY11,	//# Ready to fire laser trap
	TORSO_WEAPONREADY12,	//# Ready to fire detpack
	TORSO_WEAPONIDLE1,		//# Holding stun baton
	TORSO_WEAPONIDLE2,		//# Holding one-handed blaster
	TORSO_WEAPONIDLE3,		//# Holding blaster rifle
	TORSO_WEAPONIDLE4,		//# Holding sniper rifle
	TORSO_WEAPONIDLE5,		//# Holding bowcaster
	TORSO_WEAPONIDLE6,		//# Holding ???
	TORSO_WEAPONIDLE7,		//# Holding ???
	TORSO_WEAPONIDLE8,		//# Holding ???
	TORSO_WEAPONIDLE9,		//# Holding rocket launcher
	TORSO_WEAPONIDLE10,		//# Holding thermal det
	TORSO_WEAPONIDLE11,		//# Holding laser trap
	TORSO_WEAPONIDLE12,		//# Holding detpack

	//# #sep TORSO_ MISC
	TORSO_HANDGESTURE1,		//# gestures to left one hand
	TORSO_HANDGESTURE2,		//# gestures to right one hand
	TORSO_HANDGESTURE3,		//# gestures to the left both hands
	TORSO_HANDGESTURE4,		//# gestures to the right both hands

	TORSO_HANDEXTEND1,		//# doctor reaching for hypospray in scav5
	TORSO_HANDRETRACT1,		//# doctor taking hypospray from player in scav5

	TORSO_DROPHELMET1,		//# Drop the helmet to the waist
	TORSO_RAISEHELMET1,		//# Bring the helmet to the head
	TORSO_REACHHELMET1,		//# reaching for helmet off of 60 tall cabinet
	TORSO_GRABLBACKL,		//# reach to lower back with left hand
	TORSO_GRABUBACKL,		//# reach to upper back with left hand
	TORSO_GRABLBACKR,		//# reach to lower back with right hand
	TORSO_GRABUBACKR,		//# reach to upper back with right hand

	TORSO_SURRENDER_START,	//# arms up
	TORSO_SURRENDER_STOP,	//# arms back down

	TORSO_CHOKING1,			//# TEMP


	//=================================================
	//ANIMS IN WHICH ONLY THE LOWER OBJECTS ARE IN MD3
	//=================================================
	//# #sep Legs-only anims
	LEGS_WALKBACK1,			//# Walk1 backwards
	LEGS_WALKBACK2,			//# Walk2 backwards
	LEGS_RUNBACK1,			//# Run1 backwards
	LEGS_RUNBACK2,			//# Run2 backwards
	LEGS_TURN1,				//# What legs do when you turn your lower body to match your upper body facing
	LEGS_TURN2,				//# Leg turning from stand2
	LEGS_LEAN_LEFT1,		//# Lean left
	LEGS_LEAN_RIGHT1,		//# Lean Right
	LEGS_KNEELDOWN1,		//# Get down on one knee?
	LEGS_KNEELUP1,			//# Get up from one knee?
	LEGS_CRLEAN_LEFT1,		//# Crouch Lean left
	LEGS_CRLEAN_RIGHT1,		//# Crouch Lean Right
	LEGS_CHOKING1,			//# TEMP
	LEGS_LEFTUP1,			//# On a slope with left foot 4 higher than right
	LEGS_LEFTUP2,			//# On a slope with left foot 8 higher than right
	LEGS_LEFTUP3,			//# On a slope with left foot 12 higher than right
	LEGS_LEFTUP4,			//# On a slope with left foot 16 higher than right
	LEGS_LEFTUP5,			//# On a slope with left foot 20 higher than right
	LEGS_RIGHTUP1,			//# On a slope with RIGHT foot 4 higher than left
	LEGS_RIGHTUP2,			//# On a slope with RIGHT foot 8 higher than left
	LEGS_RIGHTUP3,			//# On a slope with RIGHT foot 12 higher than left
	LEGS_RIGHTUP4,			//# On a slope with RIGHT foot 16 higher than left
	LEGS_RIGHTUP5,			//# On a slope with RIGHT foot 20 higher than left
	LEGS_S1_LUP1,
	LEGS_S1_LUP2,
	LEGS_S1_LUP3,
	LEGS_S1_LUP4,
	LEGS_S1_LUP5,
	LEGS_S1_RUP1,
	LEGS_S1_RUP2,
	LEGS_S1_RUP3,
	LEGS_S1_RUP4,
	LEGS_S1_RUP5,
	LEGS_S3_LUP1,
	LEGS_S3_LUP2,
	LEGS_S3_LUP3,
	LEGS_S3_LUP4,
	LEGS_S3_LUP5,
	LEGS_S3_RUP1,
	LEGS_S3_RUP2,
	LEGS_S3_RUP3,
	LEGS_S3_RUP4,
	LEGS_S3_RUP5,
	LEGS_S4_LUP1,
	LEGS_S4_LUP2,
	LEGS_S4_LUP3,
	LEGS_S4_LUP4,
	LEGS_S4_LUP5,
	LEGS_S4_RUP1,
	LEGS_S4_RUP2,
	LEGS_S4_RUP3,
	LEGS_S4_RUP4,
	LEGS_S4_RUP5,
	LEGS_S5_LUP1,
	LEGS_S5_LUP2,
	LEGS_S5_LUP3,
	LEGS_S5_LUP4,
	LEGS_S5_LUP5,
	LEGS_S5_RUP1,
	LEGS_S5_RUP2,
	LEGS_S5_RUP3,
	LEGS_S5_RUP4,
	LEGS_S5_RUP5,
	//=================================================
	//HEAD ANIMS
	//=================================================
	//# #sep Head-only anims
	FACE_TALK1,			//# quiet
	FACE_TALK2,			//# semi-quiet
	FACE_TALK3,			//# semi-loud
	FACE_TALK4,			//# loud
	FACE_ALERT,				//#
	FACE_SMILE,				//#
	FACE_FROWN,				//#
	FACE_DEAD,				//#

	//# #eol
	MAX_ANIMATIONS,
} animNumber_t;

#define SABER_ANIM_GROUP_SIZE (BOTH_A2_T__B_ - BOTH_A1_T__B_)

#define MAX_ANIM_SOUNDS 69

#ifndef CG_PLAYERS_CPP

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

#else

// <"animTable" moved to cgame/animtable.h for PCH reasons>

#endif// #ifndef CG_PLAYER_CPP

// !!!!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!!!
class animFileSet_t
{
public:
	char			filename[MAX_QPATH];
	animation_t		animations[MAX_ANIMATIONS];
	animsounds_t	torsoAnimSnds[MAX_ANIM_SOUNDS];
	animsounds_t	legsAnimSnds[MAX_ANIM_SOUNDS];
	qboolean		soundsCached;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int8_t>(filename);
		saved_game.write<>(animations);
		saved_game.write<>(torsoAnimSnds);
		saved_game.write<>(legsAnimSnds);
		saved_game.write<int32_t>(soundsCached);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game)
	{
		saved_game.read<int8_t>(filename);
		saved_game.read<>(animations);
		saved_game.read<>(torsoAnimSnds);
		saved_game.read<>(legsAnimSnds);
		saved_game.read<int32_t>(soundsCached);
	}
}; // animFileSet_t
#define MAX_ANIM_FILES	64

#endif// #ifndef __ANIMS_H__

