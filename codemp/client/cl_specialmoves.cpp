/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

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

// cl_specialmoves.cpp -- the saber's special moves explained, and played from a key
//
//   specialmoves              opens or closes the menu (also in the in-game menu's TOOLS)
//   specialmoves list         the moves in the console, with how to do them by hand
//   specialmove <name>        plays the move: what keys get bound to
//
// The moves are base JKA's, decided in bg_saber.c and bg_pmove.c (RPMod keeps them). A move is
// played by putting its keys in the usercmds as if they were pressed, a step at a time: each step
// held for a time, or until the snapshot shows what it waits for (off the ground, running, the
// move begun). Only the buttons and directions are replaced, and only while a move plays; the
// mouse still turns. Moves only a .sab override reaches are listed, not played: the server's
// .sab decides those.

#include "client.h"
#include "cl_widgets.h"

#define SM_CMD			"specialmoves"
#define SM_PLAY_CMD		"specialmove"
#define SM_MAX_STEPS	6
#define SM_READY_MS		2000	// the most a saber move waits for the move before it to end
#define SM_MAX_MS		4500	// a move played longer than this is stopped
#define SM_MESSAGE_MS	5000

// the buttons a move sets or clears; the others are left as the player has them
#define SM_BUTTONS		( BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_WALKING | BUTTON_USE_HOLDABLE | BUTTON_USE \
						| BUTTON_FORCEPOWER | BUTTON_FORCEGRIP | BUTTON_FORCE_LIGHTNING | BUTTON_FORCE_DRAIN )

// styles, a bit for each saberAnimLevel
#define SB( s )			( 1 << ( s ) )
#define SM_SINGLE		( SB( SS_FAST ) | SB( SS_MEDIUM ) | SB( SS_STRONG ) | SB( SS_DESANN ) | SB( SS_TAVION ) )
#define SM_ALL			( SM_SINGLE | SB( SS_DUAL ) | SB( SS_STAFF ) )

// what a move needs
#define SM_GROUND		1	// on the ground
#define SM_AIR			2	// in the air
#define SM_SABER		4	// the saber in hand (without it, the saber or fists)
#define SM_NO_SECOND	8	// not with a staff's blade or a dual's saber off
#define SM_INFO			16	// listed, not played
#define SM_FAST			32	// Force Speed or Rage on: running backwards is too slow to roll otherwise

typedef enum {
	SM_CAT_KATA,
	SM_CAT_FORWARD,
	SM_CAT_BACK,
	SM_CAT_SIDE,
	SM_CAT_KICK,
	SM_CAT_ACRO,
	SM_CAT_AUTO,
	SM_CAT_SAB,
	SM_NUM_CATS
} smCat_t;

static const char *smCatNames[SM_NUM_CATS] = { "Katas", "Forward", "Back", "Side", "Kicks", "Acrobatics", "Automatic", ".sab only" };

// a step's end
typedef enum {
	U_END,			// no step: the move is over
	U_TIME,			// after ms
	U_AIR,			// off the ground
	U_SPEED,		// moving at arg or faster
	U_LEGS,			// the legs playing arg (or arg2)
	U_WHILE_LEGS,	// the legs no longer playing arg
	U_MOVE			// the saber move begun (the move's moves), or its animation arg or arg2 playing
} smUntil_t;

typedef struct {
	int			buttons;
	int			fwd, right, up;		// -127 to 127
	smUntil_t	until;
	int			arg, arg2;
	int			ms;					// the most it's held; at least one usercmd
	qboolean	required;			// not there by then: the rest of the move is left out
} smStep_t;

typedef struct {
	const char	*name;				// specialmove <name>
	const char	*title;
	smCat_t		cat;
	int			styles;				// SB() of the saber styles it's for, 0 any (saber or fists)
	int			flags;
	int			fp;					// force points it takes
	int			jump;				// the Force Jump level it needs
	const char	*how;				// done by hand
	const char	*desc;
	int			moves[3];			// the saber moves that show it began; -1 the style's kata
	smStep_t	steps[SM_MAX_STEPS];
} smMove_t;

#define A		BUTTON_ATTACK
#define ALT		BUTTON_ALT_ATTACK
#define FW		127
#define BK		-127
#define RT		127
#define LT		-127
#define JP		127
#define CR		-127
// a usercmd of nothing first: a jump held before lets go, so the move's jump is a new one
#define REL		{ 0, 0, 0, 0, U_TIME, 0, 0, 0, qfalse }

static const smMove_t smMoves[] = {
	// katas
	{ "kata", "Kata", SM_CAT_KATA, SM_ALL, SM_SABER | SM_GROUND | SM_NO_SECOND, 50, 0,
		"Attack and Alt Attack together (Attack a moment first), standing still",
		"Your style's special attack. Fast and Tavion: a fast spinning combo. Medium: slashes with two lunges forward. "
		"Strong and Desann: a heavy sweep that pushes forward. Dual: both sabers spin round you. Staff: the twirling Soulcal; "
		"hold a direction near its end to roll out of it.",
		{ -1 }, { REL, { A, 0, 0, 0, U_TIME, 0, 0, 30 }, { A | ALT, 0, 0, 0, U_MOVE, 0, 0, 400 } } },

	// forward
	{ "lunge", "Lunge", SM_CAT_FORWARD, SB( SS_FAST ), SM_SABER | SM_GROUND, 25, 0,
		"Crouch + Forward + Attack",
		"A hop forward from a crouch, stabbing upwards as you rise.",
		{ LS_A_LUNGE }, { REL, { A, FW, 0, CR, U_MOVE, 0, 0, 400 } } },
	{ "spinattack", "Spin attack", SM_CAT_FORWARD, SB( SS_DUAL ) | SB( SS_STAFF ), SM_SABER | SM_GROUND | SM_NO_SECOND, 25, 0,
		"Crouch + Forward + Attack",
		"A full spin with the blades out, hitting all round you.",
		{ LS_SPINATTACK, LS_SPINATTACK_DUAL }, { REL, { A, FW, 0, CR, U_MOVE, 0, 0, 400 } } },
	{ "dfa", "Death from above", SM_CAT_FORWARD, SB( SS_STRONG ), SM_SABER | SM_GROUND | SM_NO_SECOND, 25, 0,
		"Forward + Jump, Attack as you leave the ground",
		"A leap forward bringing the saber down overhead with all your weight. You can't steer it once it starts.",
		{ LS_A_JUMP_T__B_ }, { REL, { A, FW, 0, JP, U_MOVE, 0, 0, 350 } } },
	{ "flipslash", "Flip-over slash", SM_CAT_FORWARD, SB( SS_MEDIUM ), SM_SABER | SM_GROUND | SM_NO_SECOND, 25, 0,
		"Forward + Jump, Attack as you leave the ground",
		"A forward flip that slashes down at the one in front of you as you go over.",
		{ LS_A_FLIP_SLASH }, { REL, { A, FW, 0, JP, U_MOVE, 0, 0, 350 } } },
	{ "staffleap", "Twirling leap", SM_CAT_FORWARD, SB( SS_STAFF ), SM_SABER | SM_GROUND | SM_NO_SECOND, 25, 0,
		"Forward + Jump + Attack",
		"The staff's leaping butterfly: you twirl forward through the air with the staff spinning.",
		{ LS_JUMPATTACK_STAFF_RIGHT }, { REL, { A, FW, 0, JP, U_MOVE, BOTH_BUTTERFLY_FR1, 0, 350 } } },
	{ "dualleap", "Double hop attack", SM_CAT_FORWARD, SB( SS_DUAL ), SM_SABER | SM_GROUND | SM_NO_SECOND, 25, 0,
		"Forward + Jump + Attack",
		"Two short hops forward, both sabers slashing on each.",
		{ LS_JUMPATTACK_DUAL }, { REL, { A, FW, 0, JP, U_MOVE, BOTH_JUMPATTACK6, 0, 350 } } },
	{ "stabdown", "Stab down", SM_CAT_FORWARD, SM_ALL, SM_SABER | SM_GROUND | SM_NO_SECOND, 25, 0,
		"Forward + Attack, at someone knocked down in front of you",
		"Drives the saber down into someone lying on the ground in front of you (up to about 4 metres away). "
		"With nobody down there it's an ordinary overhead swing.",
		{ LS_STABDOWN, LS_STABDOWN_STAFF, LS_STABDOWN_DUAL }, { REL, { A, FW, 0, 0, U_MOVE, 0, 0, 300 } } },

	// back
	{ "backattack", "Back attack", SM_CAT_BACK, SM_ALL, SM_SABER | SM_GROUND, 0, 0,
		"Back + Attack, with someone right behind you",
		"Hits whoever is behind you without turning: a back stab with Fast and the staff, a backhand slash with the others. "
		"Only with someone within about 3 metres behind you; otherwise it's an ordinary swing.",
		{ LS_A_BACKSTAB, LS_A_BACK, LS_A_BACK_CR }, { REL, { A, BK, 0, 0, U_MOVE, 0, 0, 300 } } },
	{ "backattackcrouch", "Crouching back attack", SM_CAT_BACK,
		SB( SS_MEDIUM ) | SB( SS_STRONG ) | SB( SS_DESANN ) | SB( SS_TAVION ) | SB( SS_DUAL ), SM_SABER | SM_GROUND, 0, 0,
		"Crouch + Back + Attack, with someone right behind you",
		"The back slash done low from a crouch.",
		{ LS_A_BACK_CR, LS_A_BACK }, { REL, { A, BK, 0, CR, U_MOVE, 0, 0, 300 } } },
	{ "backflipattack", "Backflip attack", SM_CAT_BACK, SB( SS_STAFF ), SM_SABER | SM_GROUND | SM_NO_SECOND, 0, 2,
		"Back + Jump + Attack",
		"A backflip that brings the staff round over you as you go. Takes no force points, but Force Jump 2.",
		{ LS_A_BACKFLIP_ATK }, { REL, { A, BK, 0, JP, U_MOVE, BOTH_JUMPATTACK7, 0, 350 } } },

	// side
	{ "cartwheelleft", "Cartwheel left", SM_CAT_SIDE, SM_SINGLE | SB( SS_DUAL ), SM_SABER | SM_GROUND | SM_NO_SECOND, 10, 0,
		"Left + Jump + Attack",
		"An aerial cartwheel to the left, the saber sweeping as you turn over.",
		{ LS_JUMPATTACK_ARIAL_LEFT }, { REL, { A, 0, LT, JP, U_MOVE, BOTH_ARIAL_LEFT, 0, 350 } } },
	{ "cartwheelright", "Cartwheel right", SM_CAT_SIDE, SM_SINGLE | SB( SS_DUAL ), SM_SABER | SM_GROUND | SM_NO_SECOND, 10, 0,
		"Right + Jump + Attack",
		"An aerial cartwheel to the right, the saber sweeping as you turn over.",
		{ LS_JUMPATTACK_ARIAL_RIGHT }, { REL, { A, 0, RT, JP, U_MOVE, BOTH_ARIAL_RIGHT, 0, 350 } } },
	{ "butterflyleft", "Butterfly left", SM_CAT_SIDE, SB( SS_STAFF ), SM_SABER | SM_GROUND | SM_NO_SECOND, 10, 0,
		"Left + Jump + Attack",
		"The staff's butterfly kick to the left: you spin flat through the air with the staff whirling.",
		{ LS_BUTTERFLY_LEFT }, { REL, { A, 0, LT, JP, U_MOVE, BOTH_BUTTERFLY_LEFT, 0, 350 } } },
	{ "butterflyright", "Butterfly right", SM_CAT_SIDE, SB( SS_STAFF ), SM_SABER | SM_GROUND | SM_NO_SECOND, 10, 0,
		"Right + Jump + Attack",
		"The staff's butterfly kick to the right: you spin flat through the air with the staff whirling.",
		{ LS_BUTTERFLY_RIGHT }, { REL, { A, 0, RT, JP, U_MOVE, BOTH_BUTTERFLY_RIGHT, 0, 350 } } },

	// kicks
	{ "kickfront", "Front kick", SM_CAT_KICK, SB( SS_STAFF ), SM_SABER, 0, 0,
		"Forward + Alt Attack",
		"With the staff, Alt Attack kicks instead of throwing. Done in the air it's an air kick.",
		{ LS_KICK_F, LS_KICK_F_AIR }, { REL, { ALT, FW, 0, 0, U_MOVE, 0, 0, 300 } } },
	{ "kickback", "Back kick", SM_CAT_KICK, SB( SS_STAFF ), SM_SABER, 0, 0,
		"Back + Alt Attack",
		"A kick behind you without turning round.",
		{ LS_KICK_B, LS_KICK_B_AIR }, { REL, { ALT, BK, 0, 0, U_MOVE, 0, 0, 300 } } },
	{ "kickleft", "Side kick left", SM_CAT_KICK, SB( SS_STAFF ), SM_SABER, 0, 0,
		"Left + Alt Attack",
		"A kick to your left.",
		{ LS_KICK_L, LS_KICK_L_AIR }, { REL, { ALT, 0, LT, 0, U_MOVE, 0, 0, 300 } } },
	{ "kickright", "Side kick right", SM_CAT_KICK, SB( SS_STAFF ), SM_SABER, 0, 0,
		"Right + Alt Attack",
		"A kick to your right.",
		{ LS_KICK_R, LS_KICK_R_AIR }, { REL, { ALT, 0, RT, 0, U_MOVE, 0, 0, 300 } } },

	// acrobatics
	{ "rollstab", "Roll stab", SM_CAT_ACRO, SM_ALL, SM_SABER | SM_GROUND, 25, 1,
		"Run, Crouch to roll forward, Attack at the end of the roll",
		"A forward roll that comes up stabbing. The attack has to land in the last quarter second of the roll.",
		{ LS_ROLL_STAB }, { REL, { 0, FW, 0, 0, U_SPEED, 235, 0, 900 }, { 0, FW, 0, CR, U_LEGS, BOTH_ROLL_F, 0, 400, qtrue },
			{ A, 0, 0, 0, U_MOVE, 0, 0, 1200 } } },
	{ "rollforward", "Roll forward", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Run forward, then Crouch",
		"A forward roll out of a run.",
		{ 0 }, { REL, { 0, FW, 0, 0, U_SPEED, 235, 0, 900 }, { 0, FW, 0, CR, U_LEGS, BOTH_ROLL_F, 0, 400 } } },
	{ "rollback", "Roll back", SM_CAT_ACRO, 0, SM_GROUND | SM_FAST, 0, 1,
		"Run backwards with Force Speed or Rage on, then Crouch",
		"A backward roll out of a backward run. Running backwards is a quarter slower than forwards, too slow to roll, "
		"so it only works with Force Speed or Rage on.",
		{ 0 }, { REL, { 0, BK, 0, 0, U_SPEED, 205, 0, 900 }, { 0, BK, 0, CR, U_LEGS, BOTH_ROLL_B, 0, 400 } } },
	{ "rollleft", "Roll left", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Run left, then Crouch",
		"A sideways roll to the left.",
		{ 0 }, { REL, { 0, 0, LT, 0, U_SPEED, 235, 0, 900 }, { 0, 0, LT, CR, U_LEGS, BOTH_ROLL_L, 0, 400 } } },
	{ "rollright", "Roll right", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Run right, then Crouch",
		"A sideways roll to the right.",
		{ 0 }, { REL, { 0, 0, RT, 0, U_SPEED, 235, 0, 900 }, { 0, 0, RT, CR, U_LEGS, BOTH_ROLL_R, 0, 400 } } },
	{ "backflip", "Back flip", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Back + Jump",
		"A flip backwards from standing.",
		{ 0 }, { REL, { 0, BK, 0, JP, U_LEGS, BOTH_FLIP_BACK1, 0, 250 } } },
	{ "flipforward", "Flip forward", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Forward + hold Jump",
		"A forward flip at the top of a force jump: keep Jump held until it turns into the flip.",
		{ 0 }, { REL, { 0, FW, 0, JP, U_LEGS, BOTH_FLIP_F, 0, 900 } } },
	{ "flipleft", "Flip left", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Left + hold Jump",
		"A sideways flip to the left at the top of a force jump. Next to a wall on your left it's a wall flip instead.",
		{ 0 }, { REL, { 0, 0, LT, JP, U_LEGS, BOTH_FLIP_L, BOTH_WALL_FLIP_LEFT, 900 } } },
	{ "flipright", "Flip right", SM_CAT_ACRO, 0, SM_GROUND, 0, 1,
		"Right + hold Jump",
		"A sideways flip to the right at the top of a force jump. Next to a wall on your right it's a wall flip instead.",
		{ 0 }, { REL, { 0, 0, RT, JP, U_LEGS, BOTH_FLIP_R, BOTH_WALL_FLIP_RIGHT, 900 } } },
	{ "wallrunleft", "Wall run left", SM_CAT_ACRO, 0, SM_GROUND, 0, 2,
		"Forward + Left + Jump, beside a wall on your left",
		"Runs along a wall on your left. Jump again while running to flip off it.",
		{ 0 }, { REL, { 0, FW, LT, JP, U_LEGS, BOTH_WALL_RUN_LEFT, 0, 300, qtrue }, { 0, FW, LT, 0, U_TIME, 0, 0, 300 } } },
	{ "wallrunright", "Wall run right", SM_CAT_ACRO, 0, SM_GROUND, 0, 2,
		"Forward + Right + Jump, beside a wall on your right",
		"Runs along a wall on your right. Jump again while running to flip off it.",
		{ 0 }, { REL, { 0, FW, RT, JP, U_LEGS, BOTH_WALL_RUN_RIGHT, 0, 300, qtrue }, { 0, FW, RT, 0, U_TIME, 0, 0, 300 } } },
	{ "wallflipleft", "Wall flip left", SM_CAT_ACRO, 0, SM_GROUND, 0, 2,
		"Left + Jump, beside a wall on your left",
		"Kicks off a wall on your left into a flip away from it.",
		{ 0 }, { REL, { 0, 0, LT, JP, U_LEGS, BOTH_WALL_FLIP_LEFT, 0, 300 } } },
	{ "wallflipright", "Wall flip right", SM_CAT_ACRO, 0, SM_GROUND, 0, 2,
		"Right + Jump, beside a wall on your right",
		"Kicks off a wall on your right into a flip away from it.",
		{ 0 }, { REL, { 0, 0, RT, JP, U_LEGS, BOTH_WALL_FLIP_RIGHT, 0, 300 } } },
	{ "wallrunup", "Wall run up", SM_CAT_ACRO, 0, SM_GROUND, 0, 2,
		"Forward + Jump at a wall, Jump again in the air",
		"Runs up the wall in front of you. With Force Jump 2 you backflip off it; with Force Jump 3 you climb it as long as "
		"you hold Forward, and Jump flips you back off.",
		{ 0 }, { REL, { 0, FW, 0, JP, U_AIR, 0, 0, 300, qtrue }, { 0, FW, 0, 0, U_TIME, 0, 0, 60 },
			{ 0, FW, 0, JP, U_LEGS, BOTH_FORCEWALLRUNFLIP_START, BOTH_WALL_FLIP_BACK1, 250, qtrue },
			{ 0, FW, 0, 0, U_WHILE_LEGS, BOTH_FORCEWALLRUNFLIP_START, 0, 2500 } } },
	{ "wallgrab", "Wall grab", SM_CAT_ACRO, 0, SM_AIR, 0, 3,
		"In the air: Forward + Jump at a wall in front of you",
		"Catches the wall in front of you in mid-air, then pushes off it. Use it while you're in the air.",
		{ 0 }, { REL, { 0, FW, 0, JP, U_TIME, 0, 0, 150 } } },

	// automatic, or done by hand
	{ "mirror", "Mirror attacks", SM_CAT_AUTO, SB( SS_DUAL ), SM_INFO, 0, 0,
		"Automatic with two sabers",
		"With enemies on both sides, a left or right swing hits both at once; with enemies in front and behind, an "
		"overhead or back swing does. Nothing to press: it happens when they're there." },
	{ "saberthrow", "Saber throw", SM_CAT_AUTO, SM_SINGLE | SB( SS_DUAL ), SM_INFO, 20, 0,
		"Alt Attack (hold it to steer, let go to call it back)",
		"Throws your saber. With two sabers one flies while you fight on with the other. Needs Saber Throw 1." },
	{ "getuproll", "Getting up", SM_CAT_AUTO, 0, SM_INFO, 0, 0,
		"When knocked down: a direction, or Jump",
		"As you get up from a knockdown, holding a direction rolls you that way, and Jump flips you straight up." },
	{ "saberlock", "Saber lock", SM_CAT_AUTO, 0, SM_INFO, 0, 0,
		"Tap Attack quickly",
		"Locked blade to blade, each new press of Attack pushes; stronger styles push more for each press." },

	// only a .sab reaches these
	{ "sabkata", "Another style's kata", SM_CAT_SAB, 0, SM_INFO, 0, 0,
		"kataMove in the saber's .sab",
		"A saber's .sab can give it any kata, the staff's Soulcal on a single saber for one, or take it away (LS_NONE). "
		"It's the server's copy of the .sab that counts, so a client can't change it." },
	{ "sabjump", "Other lunges and jump attacks", SM_CAT_SAB, 0, SM_INFO, 0, 0,
		"lungeAtkMove, jumpAtkFwdMove, jumpAtkBackMove, jumpAtkLeftMove, jumpAtkRightMove",
		"A .sab can put another move in the place of the lunge or spin, the forward jump attacks, the staff's backflip "
		"attack and the cartwheels. jumpAtkUpMove is read but never used." },
	{ "sabunused", "Moves left out of the game", SM_CAT_SAB, 0, SM_INFO, 0, 0,
		"Only as a .sab's move",
		"The game has these moves but nothing in multiplayer starts them: the pull attacks, the leap attack, the upside-down "
		"attack, Alora's spin, the hilt bash, the kick combos and the cartwheel and flip stab versions. A .sab can still "
		"name one in place of a move above." },
};
#define SM_NUM_MOVES	( (int)ARRAY_LEN( smMoves ) )

#undef A
#undef ALT
#undef FW
#undef BK
#undef RT
#undef LT
#undef JP
#undef CR
#undef REL

static const char *smStyleNames[] = { "", "Fast", "Medium", "Strong", "Desann", "Tavion", "Dual", "Staff" };

/*
===============================================================================

WHAT YOU CAN DO NOW

===============================================================================
*/

static const smMove_t *SM_Find( const char *name ) {
	for ( int i = 0; i < SM_NUM_MOVES; i++ ) {
		if ( !Q_stricmp( smMoves[i].name, name ) )
			return &smMoves[i];
	}
	return NULL;
}

// "Fast, Medium and Staff"
static const char *SM_StyleList( int styles ) {
	static char out[128];
	int n = 0, count = 0;

	if ( !styles )
		return "any style, or fists";
	if ( styles == SM_ALL )
		return "every style";
	for ( int s = SS_FAST; s <= SS_STAFF; s++ )
		count += ( styles & SB( s ) ) ? 1 : 0;
	out[0] = '\0';
	for ( int s = SS_FAST; s <= SS_STAFF; s++ ) {
		if ( !( styles & SB( s ) ) )
			continue;
		n++;
		Q_strcat( out, sizeof( out ), va( "%s%s", n == 1 ? "" : n == count ? " and " : ", ", smStyleNames[s] ) );
	}
	return out;
}

// the style's kata
static int SM_KataMove( int style ) {
	switch ( style ) {
	case SS_FAST:
	case SS_TAVION:		return LS_A1_SPECIAL;
	case SS_MEDIUM:		return LS_A2_SPECIAL;
	case SS_STRONG:
	case SS_DESANN:		return LS_A3_SPECIAL;
	case SS_DUAL:		return LS_DUAL_SPIN_PROTECT;
	case SS_STAFF:		return LS_STAFF_SOULCAL;
	default:			return LS_NONE;
	}
}

// in game and playing: not dead, spectating, following or in a vehicle
static const char *SM_NotPlaying( void ) {
	const playerState_t *ps = &cl.snap.ps;

	if ( cls.state != CA_ACTIVE || !cl.snap.valid || clc.demoplaying )
		return "Join a server first";
	if ( ( ps->pm_flags & PMF_FOLLOW ) || ps->pm_type == PM_SPECTATOR )
		return "Not while spectating";
	if ( ps->stats[STAT_HEALTH] <= 0 || ps->pm_type == PM_DEAD )
		return "Not while dead";
	if ( ps->m_iVehicleNum )
		return "Not in a vehicle";
	if ( ps->pm_type != PM_NORMAL )
		return "Not now: you can't move freely";
	return NULL;
}

// the Force Jump level: the server sends it in 2 bits, so 4 comes as 0; Force Jump known at 0 is that 4
static int SM_JumpLevel( const playerState_t *ps ) {
	const int level = ps->fd.forcePowerLevel[FP_LEVITATION];

	if ( level == 0 && ( ps->fd.forcePowersKnown & ( 1 << FP_LEVITATION ) ) )
		return 4;
	return level;
}

// why you can't do it now, or NULL
static const char *SM_CantDo( const smMove_t *m ) {
	const playerState_t *ps = &cl.snap.ps;
	const char *why = SM_NotPlaying();
	const int style = ps->fd.saberAnimLevel;

	if ( m->flags & SM_INFO )
		return m->cat == SM_CAT_SAB ? "Only through a .sab: can't be played from a key" : "Done by hand, or happens by itself";
	if ( why )
		return why;
	if ( ( m->flags & SM_SABER ) && ps->weapon != WP_SABER )
		return "Take out your saber";
	if ( !( m->flags & SM_SABER ) && ps->weapon != WP_SABER && ps->weapon != WP_MELEE )
		return "Only with the saber or fists";
	if ( m->styles && ps->weapon == WP_SABER && !( m->styles & SB( style ) ) )
		return va( "Not with %s: only %s", style > 0 && style < (int)ARRAY_LEN( smStyleNames ) ? smStyleNames[style] : "this style",
			SM_StyleList( m->styles ) );
	if ( ( m->flags & SM_NO_SECOND ) && ( ps->fd.saberAnimLevelBase == SS_DUAL || ps->fd.saberAnimLevelBase == SS_STAFF )
		&& style != ps->fd.saberAnimLevelBase )
		return ps->fd.saberAnimLevelBase == SS_STAFF ? "Turn the staff's other blade on" : "Turn your other saber on";
	if ( ( m->flags & SM_SABER ) && ps->saberHolstered == 2 )
		return "Turn your saber on";
	if ( ( m->flags & SM_FAST ) && !( ps->fd.forcePowersActive & ( ( 1 << FP_SPEED ) | ( 1 << FP_RAGE ) ) ) )
		return "Needs Force Speed or Rage on";
	if ( m->jump && SM_JumpLevel( ps ) < m->jump )
		return va( "Needs Force Jump %i (you have %i)", m->jump, SM_JumpLevel( ps ) );
	if ( m->fp && ps->fd.forcePower < m->fp )
		return va( "Needs %i force points (you have %i)", m->fp, ps->fd.forcePower );
	if ( ( m->flags & SM_GROUND ) && ps->groundEntityNum == ENTITYNUM_NONE )
		return "Only from the ground";
	if ( ( m->flags & SM_AIR ) && ps->groundEntityNum != ENTITYNUM_NONE )
		return "Only in the air";
	return NULL;
}

/*
===============================================================================

PLAYING A MOVE

===============================================================================
*/

static struct {
	const smMove_t	*move;			// playing, NULL if none
	int				step;
	int				stepStart;
	int				stepCmds;		// usercmds the step has been in
	int				start;
	int				weapon;
	int				style;			// the kata's style
	qboolean		waitReady;		// a saber move waiting for the swing before it to end
} sp;

static cvar_t *cl_specialMovesDebug;

// what the snapshot shows now, for cl_specialMovesDebug
static const char *SM_DebugState( void ) {
	const playerState_t *ps = &cl.snap.ps;

	return va( "pm_type %i pm_flags 0x%x health %i saberMove %i legsAnim %i weapon %i style %i fp %i jump %i %s speed %.0f",
		ps->pm_type, ps->pm_flags, ps->stats[STAT_HEALTH], ps->saberMove, ps->legsAnim, ps->weapon,
		ps->fd.saberAnimLevel, ps->fd.forcePower, SM_JumpLevel( ps ),
		ps->groundEntityNum == ENTITYNUM_NONE ? "air" : "ground", sqrtf( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] ) );
}

static void SM_Stop( void ) {
	if ( sp.move && cl_specialMovesDebug && cl_specialMovesDebug->integer )
		Com_Printf( "specialmove %s: over after %i ms (%s)\n", sp.move->name, cls.realtime - sp.start, SM_DebugState() );
	sp.move = NULL;
}

static qboolean SM_Begun( const smMove_t *m, int saberMove ) {
	for ( int i = 0; i < (int)ARRAY_LEN( m->moves ); i++ ) {
		if ( m->moves[i] == -1 && saberMove == SM_KataMove( sp.style ) )
			return qtrue;
		if ( m->moves[i] > 0 && saberMove == m->moves[i] )
			return qtrue;
	}
	return qfalse;
}

// 0 going on, 1 done, 2 out of time
static int SM_StepState( const smMove_t *m, const smStep_t *s ) {
	const playerState_t *ps = &cl.snap.ps;
	qboolean done = qfalse;

	switch ( s->until ) {
	case U_AIR:
		done = (qboolean)( ps->groundEntityNum == ENTITYNUM_NONE );
		break;
	case U_SPEED:
		done = (qboolean)( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] >= (float)s->arg * s->arg );
		break;
	case U_LEGS:
		done = (qboolean)( ps->legsAnim == s->arg || ( s->arg2 && ps->legsAnim == s->arg2 ) );
		break;
	case U_WHILE_LEGS:
		done = (qboolean)( ps->legsAnim != s->arg );
		break;
	case U_MOVE:
		// a short move can be over between two snapshots: its animation shows it too
		done = (qboolean)( SM_Begun( m, ps->saberMove ) || ( s->arg && ( ps->legsAnim == s->arg || ps->torsoAnim == s->arg ) )
			|| ( s->arg2 && ( ps->legsAnim == s->arg2 || ps->torsoAnim == s->arg2 ) ) );
		break;
	default:
		break;
	}
	if ( done )
		return 1;
	return cls.realtime - sp.stepStart >= s->ms ? 2 : 0;
}

static void SM_Play( const smMove_t *m ) {
	sp.move = m;
	sp.step = 0;
	sp.stepStart = sp.start = cls.realtime;
	sp.stepCmds = 0;
	sp.weapon = cl.snap.ps.weapon;
	sp.style = cl.snap.ps.fd.saberAnimLevel;
	sp.waitReady = (qboolean)( ( m->flags & SM_SABER ) != 0 );
	if ( cl_specialMovesDebug->integer )
		Com_Printf( "specialmove %s: start (%s)\n", m->name, SM_DebugState() );
}

// each usercmd while a move plays: its step's buttons and directions in place of the player's
void CL_SpecialMoves_FilterCmd( usercmd_t *cmd ) {
	const smMove_t *m = sp.move;
	const smStep_t *s;

	if ( !m )
		return;
	if ( SM_NotPlaying() || Key_GetCatcher() || cl.snap.ps.weapon != sp.weapon || cls.realtime - sp.start > SM_MAX_MS ) {
		if ( cl_specialMovesDebug->integer )
			Com_Printf( "specialmove %s: stopped (%s)\n", m->name, SM_NotPlaying() ? SM_NotPlaying() : Key_GetCatcher() ? "a menu or the console opened"
				: cl.snap.ps.weapon != sp.weapon ? "the weapon changed" : "too long" );
		SM_Stop();
		return;
	}
	// a saber move starts once the saber is ready: till then the player's keys go as they are
	if ( sp.waitReady ) {
		const playerState_t *ps = &cl.snap.ps;

		if ( ( ps->weaponTime > 0 || ( ps->saberMove != LS_READY && ps->saberMove != LS_NONE ) ) && cls.realtime - sp.start < SM_READY_MS )
			return;
		sp.waitReady = qfalse;
		sp.stepStart = cls.realtime;
		if ( cl_specialMovesDebug->integer )
			Com_Printf( "specialmove %s: ready after %i ms (%s)\n", m->name, cls.realtime - sp.start, SM_DebugState() );
	}
	for ( ;; ) {
		int state;

		if ( sp.step >= SM_MAX_STEPS || m->steps[sp.step].until == U_END ) {
			SM_Stop();
			return;
		}
		s = &m->steps[sp.step];
		// each step is in one usercmd at least
		state = sp.stepCmds ? SM_StepState( m, s ) : 0;
		if ( !state )
			break;
		if ( cl_specialMovesDebug->integer )
			Com_Printf( "specialmove %s: step %i %s after %i ms (%s)\n", m->name, sp.step, state == 1 ? "done" : "out of time",
				cls.realtime - sp.stepStart, SM_DebugState() );
		if ( state == 2 && s->required ) {
			SM_Stop();
			return;
		}
		sp.step++;
		sp.stepStart = cls.realtime;
		sp.stepCmds = 0;
	}
	cmd->buttons = ( cmd->buttons & ~SM_BUTTONS ) | s->buttons;
	cmd->forwardmove = (signed char)s->fwd;
	cmd->rightmove = (signed char)s->right;
	cmd->upmove = (signed char)s->up;
	sp.stepCmds++;
}

// specialmove <name>
static void SM_Play_f( void ) {
	const smMove_t *m;
	const char *why;

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "usage: " SM_PLAY_CMD " <move>   plays a special move (" SM_CMD " shows them all):\n " );
		for ( int i = 0; i < SM_NUM_MOVES; i++ ) {
			if ( !( smMoves[i].flags & SM_INFO ) )
				Com_Printf( " %s", smMoves[i].name );
		}
		Com_Printf( "\n" );
		return;
	}
	m = SM_Find( Cmd_Argv( 1 ) );
	if ( !m ) {
		Com_Printf( S_COLOR_YELLOW "There's no special move %s (" SM_CMD " list shows them)\n", Cmd_Argv( 1 ) );
		return;
	}
	why = SM_CantDo( m );
	if ( why ) {
		Com_Printf( S_COLOR_YELLOW "%s: %s\n", m->title, why );
		if ( cl_specialMovesDebug->integer )
			Com_Printf( "specialmove %s: refused (%s)\n", m->name, SM_DebugState() );
		return;
	}
	SM_Play( m );
}

/*
===============================================================================

THE MENU

===============================================================================
*/

#define SM_X			40.0f
#define SM_Y			34.0f
#define SM_W			560.0f
#define SM_H			412.0f
#define SM_CAT_X		( SM_X + 12 )
#define SM_CAT_W		104.0f
#define SM_LIST_X		( SM_CAT_X + SM_CAT_W + 10 )
#define SM_LIST_W		176.0f
#define SM_DETAIL_X		( SM_LIST_X + SM_LIST_W + 12 )
#define SM_DETAIL_W		( SM_X + SM_W - 12 - SM_DETAIL_X )
#define SM_TOP			( SM_Y + 54 )
#define SM_ROW_H		20.0f
#define SM_LIST_H		( SM_ROW_H * 15 )
#define SM_CTRL_H		16.0f
#define SM_TEXT_SCALE	0.8f
#define SM_ITEM_SCALE	0.6f

// JKA's menu colours, as the patched menus use them
static const float smPanel[4]		= { 0.0f, 0.0f, 0.08f, 0.88f };
static const float smBorder[4]		= { 0.5f, 0.5f, 0.7f, 1.0f };
static const float smItem[4]		= { 0.15f, 0.15f, 0.25f, 0.8f };
static const float smItemHover[4]	= { 0.25f, 0.25f, 0.4f, 0.9f };
static const float smItemOn[4]		= { 0.35f, 0.24f, 0.05f, 0.9f };
static const float smOrange[4]		= { 1.0f, 0.682f, 0.0f, 1.0f };
static const float smWhite[4]		= { 1.0f, 1.0f, 1.0f, 1.0f };
static const float smDim[4]			= { 0.6f, 0.6f, 0.7f, 1.0f };
static const float smGreen[4]		= { 0.4f, 1.0f, 0.4f, 1.0f };
static const float smYellow[4]		= { 1.0f, 0.85f, 0.3f, 1.0f };

static struct {
	qboolean	open;
	int			font, titleFont;
	sfxHandle_t	clickSound;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			cat;
	int			pick;			// the move picked, its index in smMoves
	int			scroll;
	qboolean	binding;		// waiting on a key to bind
	char		message[MAX_STRING_CHARS];
	int			messageTime;
} sm;

static void SM_Message( const char *text ) {
	Q_strncpyz( sm.message, text, sizeof( sm.message ) );
	sm.messageTime = cls.realtime;
}

// the moves of the category shown, as indexes in smMoves; how many
static int SM_Shown( int *list ) {
	int n = 0;

	for ( int i = 0; i < SM_NUM_MOVES; i++ ) {
		if ( smMoves[i].cat == sm.cat )
			list[n++] = i;
	}
	return n;
}

static void SM_SetCat( int cat ) {
	int list[SM_NUM_MOVES];

	sm.cat = cat;
	sm.scroll = 0;
	sm.binding = qfalse;
	sm.pick = SM_Shown( list ) ? list[0] : -1;
}

static void SM_Close( void ) {
	if ( !sm.open )
		return;
	sm.open = qfalse;
	sm.binding = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_SPECIALMOVES );
}

void CL_SpecialMoves_Close( void ) {
	SM_Close();
}

static void SM_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Special moves: join a server first\n" );
		return;
	}
	if ( sm.open )
		return;
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_MapEditor_Close();
	CL_SoundManager_Close();
	CL_AnimManager_Close();
	CL_Presets_Close();
	CL_AssetManager_Close();
	CL_RadialMenu_CloseSettings();
	CL_Languages_Close();
	CL_ChatLog_Close();

	sm.font = re->RegisterFont( "arialnb" );
	if ( !sm.font )
		sm.font = cls.menuFont;
	sm.titleFont = re->RegisterFont( "ergoec" );
	if ( !sm.titleFont )
		sm.titleFont = sm.font;
	sm.clickSound = S_RegisterSound( "sound/interface/button1.wav" );
	sm.cursorX = SCREEN_WIDTH * 0.5f;
	sm.cursorY = SCREEN_HEIGHT * 0.5f;
	sm.click = qfalse;
	if ( sm.pick < 0 || sm.pick >= SM_NUM_MOVES || smMoves[sm.pick].cat != sm.cat )
		SM_SetCat( sm.cat );
	sm.open = qtrue;
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_SPECIALMOVES );
}

// "F, MOUSE4": the keys that play it
static const char *SM_Keys( const smMove_t *m ) {
	static char keys[MAX_STRING_CHARS];
	char want[MAX_QPATH];

	Com_sprintf( want, sizeof( want ), SM_PLAY_CMD " %s", m->name );
	keys[0] = '\0';
	for ( int i = 0; i < MAX_KEYS; i++ ) {
		const char *binding = Key_GetBinding( i );

		if ( VALIDSTRING( binding ) && !Q_stricmp( binding, want ) )
			Q_strcat( keys, sizeof( keys ), va( "%s%s", keys[0] ? ", " : "", Key_KeynumToString( i ) ) );
	}
	return keys;
}

static void SM_Unbind( const smMove_t *m ) {
	char want[MAX_QPATH];
	int n = 0;

	Com_sprintf( want, sizeof( want ), SM_PLAY_CMD " %s", m->name );

	for ( int i = 0; i < MAX_KEYS; i++ ) {
		const char *binding = Key_GetBinding( i );

		if ( VALIDSTRING( binding ) && !Q_stricmp( binding, want ) ) {
			Key_SetBinding( i, "" );
			n++;
		}
	}
	SM_Message( n ? va( "Unbound %i key%s", n, n == 1 ? "" : "s" ) : "No key plays it" );
}

// specialmoves [list|close]
static void SM_Cmd_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "list" ) ) {
		for ( int c = 0; c < SM_NUM_CATS; c++ ) {
			Com_Printf( S_COLOR_YELLOW "%s\n", smCatNames[c] );
			for ( int i = 0; i < SM_NUM_MOVES; i++ ) {
				const smMove_t *m = &smMoves[i];

				if ( m->cat != c )
					continue;
				Com_Printf( "  %-18s %s: %s (%s%s%s)\n", ( m->flags & SM_INFO ) ? "" : m->name, m->title, m->how, SM_StyleList( m->styles ),
					m->fp ? va( ", %i force points", m->fp ) : "", m->jump ? va( ", Force Jump %i", m->jump ) : "" );
			}
		}
		Com_Printf( "Bind one with: bind <key> \"" SM_PLAY_CMD " <name>\"\n" );
	} else if ( !Q_stricmp( arg, "close" ) ) {
		SM_Close();
	} else if ( arg[0] ) {
		Com_Printf( "usage: " SM_CMD "         open or close the menu\n" );
		Com_Printf( "       " SM_CMD " list    the moves and how to do them\n" );
		Com_Printf( "       " SM_PLAY_CMD " <name>   plays one (bind keys to it)\n" );
	} else if ( sm.open ) {
		SM_Close();
	} else {
		SM_Open();
	}
}

static void SM_ClickSound( void ) {
	if ( sm.clickSound )
		S_StartLocalSound( sm.clickSound, CHAN_LOCAL_SOUND );
}

static qboolean SM_IsToggleKey( int key ) {
	const char *binding = Key_GetBinding( key );

	return (qboolean)( VALIDSTRING( binding ) && !Q_stricmp( binding, SM_CMD ) );
}

static void SM_Step( int dir ) {
	int list[SM_NUM_MOVES], n = SM_Shown( list ), at = 0;

	if ( !n )
		return;
	for ( int i = 0; i < n; i++ ) {
		if ( list[i] == sm.pick )
			at = i;
	}
	at = Com_Clampi( 0, n - 1, at + dir );
	sm.pick = list[at];
	sm.binding = qfalse;
	if ( at < sm.scroll )
		sm.scroll = at;
	else if ( at >= sm.scroll + (int)( SM_LIST_H / SM_ROW_H ) )
		sm.scroll = at - (int)( SM_LIST_H / SM_ROW_H ) + 1;
}

void CL_SpecialMoves_KeyEvent( int key, qboolean down ) {
	if ( !sm.open || !down )
		return;
	// waiting on a key to bind: the mouse's two buttons still click
	if ( sm.binding && key != A_MOUSE1 && key != A_MOUSE2 && sm.pick >= 0 ) {
		sm.binding = qfalse;
		Key_SetBinding( key, va( SM_PLAY_CMD " %s", smMoves[sm.pick].name ) );
		SM_Message( va( S_COLOR_GREEN "%s plays %s", Key_KeynumToString( key ), smMoves[sm.pick].title ) );
		return;
	}
	if ( key == A_MOUSE1 ) {
		sm.click = qtrue;
		sm.clickX = sm.cursorX;
		sm.clickY = sm.cursorY;
		return;
	}
	if ( SM_IsToggleKey( key ) ) {
		SM_Close();
		return;
	}
	switch ( key ) {
	case A_CURSOR_UP:
		SM_Step( -1 );
		break;
	case A_CURSOR_DOWN:
		SM_Step( 1 );
		break;
	case A_CURSOR_LEFT:
		SM_SetCat( ( sm.cat + SM_NUM_CATS - 1 ) % SM_NUM_CATS );
		break;
	case A_CURSOR_RIGHT:
	case A_TAB:
		SM_SetCat( ( sm.cat + 1 ) % SM_NUM_CATS );
		break;
	case A_MWHEELUP:
		sm.scroll = Q_max( 0, sm.scroll - 1 );
		break;
	case A_MWHEELDOWN:
		sm.scroll++;	// held in range as it's drawn
		break;
	default:
		break;
	}
}

void CL_SpecialMoves_MouseEvent( int dx, int dy ) {
	sm.cursorX = Com_Clamp( 0, SCREEN_WIDTH, sm.cursorX + dx );
	sm.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, sm.cursorY + dy );
}

// a bind waited on first, then closed
void CL_SpecialMoves_Escape( void ) {
	if ( sm.binding ) {
		sm.binding = qfalse;
		SM_Message( "Not bound" );
		return;
	}
	SM_Close();
}

static void SM_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void SM_Box( float x, float y, float w, float h, const float *fill ) {
	SM_Fill( x, y, w, h, fill );
	SM_Fill( x, y, w, 1, smBorder );
	SM_Fill( x, y + h - 1, w, 1, smBorder );
	SM_Fill( x, y, 1, h, smBorder );
	SM_Fill( x + w - 1, y, 1, h, smBorder );
}

// cut to fit w, centred in it when centred
static void SM_Text( float x, float y, float w, const char *text, const float *color, int font, float scale, qboolean centred ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && re->Font_StrLenPixels( buf, font, scale ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	if ( centred )
		x += ( w - re->Font_StrLenPixels( buf, font, scale ) ) * 0.5f;
	re->Font_DrawString( (int)x, (int)y, buf, color, font | STYLE_DROPSHADOW, -1, scale );
}

static qboolean SM_InRect( float x, float y, float w, float h ) {
	return (qboolean)( sm.cursorX >= x && sm.cursorX < x + w && sm.cursorY >= y && sm.cursorY < y + h );
}

static qboolean SM_Clicked( float x, float y, float w, float h ) {
	if ( !sm.click || sm.clickX < x || sm.clickX >= x + w || sm.clickY < y || sm.clickY >= y + h )
		return qfalse;
	sm.click = qfalse;
	SM_ClickSound();
	return qtrue;
}

static qboolean SM_Button( float x, float y, float w, const char *label, qboolean on, qboolean enabled ) {
	SM_Box( x, y, w, SM_CTRL_H, on ? smItemOn : enabled && SM_InRect( x, y, w, SM_CTRL_H ) ? smItemHover : smItem );
	SM_Text( x, y + 2, w, label, enabled ? smWhite : smDim, sm.font, SM_TEXT_SCALE, qtrue );
	return enabled ? SM_Clicked( x, y, w, SM_CTRL_H ) : qfalse;
}

// wrapped text at x, y in w: the height it took
static float SM_Para( float x, float y, float w, const char *text, const float *color ) {
	wgStyle_t style;
	int lines;

	memset( &style, 0, sizeof( style ) );
	style.font = sm.font;
	style.scale = SM_TEXT_SCALE;
	style.lineH = 13.0f;
	style.text = color;
	lines = WG_TextLines( &style, text, w );
	WG_DrawText( &style, x, y, w, text, 0, lines, color );
	return lines * style.lineH;
}

// a label and its text after it, wrapped under
static float SM_Field( float x, float y, float w, const char *label, const char *text, const float *color ) {
	SM_Text( x, y, w, label, smOrange, sm.font, SM_TEXT_SCALE, qfalse );
	return 13.0f + SM_Para( x + 8, y + 13, w - 8, text, color ) + 4.0f;
}

static void SM_DrawState( void ) {
	const playerState_t *ps = &cl.snap.ps;
	const int style = ps->fd.saberAnimLevel;
	const char *text;

	if ( SM_NotPlaying() )
		text = SM_NotPlaying();
	else if ( ps->weapon != WP_SABER )
		text = va( "No saber out    Force points %i    Force Jump %i", ps->fd.forcePower, SM_JumpLevel( ps ) );
	else
		text = va( "Style %s    Force points %i    Force Jump %i",
			style > 0 && style < (int)ARRAY_LEN( smStyleNames ) ? smStyleNames[style] : "?", ps->fd.forcePower, SM_JumpLevel( ps ) );
	SM_Text( SM_CAT_X, SM_Y + 34, SM_W - 24, text, smDim, sm.font, SM_TEXT_SCALE, qfalse );
}

static void SM_DrawDetail( void ) {
	const float x = SM_DETAIL_X, w = SM_DETAIL_W;
	const smMove_t *m;
	const char *why, *keys;
	float y = SM_TOP;

	SM_Box( x - 6, SM_TOP - 4, w + 12, SM_LIST_H + 8, smItem );
	if ( sm.pick < 0 )
		return;
	m = &smMoves[sm.pick];
	SM_Text( x, y, w, m->title, smOrange, sm.titleFont, SM_ITEM_SCALE + 0.1f, qfalse );
	y += 24;
	y += SM_Field( x, y, w, "How", m->how, smWhite );
	if ( !( m->flags & SM_INFO ) || m->styles )
		y += SM_Field( x, y, w, "Styles", SM_StyleList( m->styles ), smWhite );
	if ( m->fp || m->jump )
		y += SM_Field( x, y, w, "Needs", va( "%s%s%s", m->fp ? va( "%i force points", m->fp ) : "", m->fp && m->jump ? ", " : "",
			m->jump ? va( "Force Jump %i", m->jump ) : "" ), smWhite );
	y += 4;
	y += SM_Para( x, y, w, m->desc, smWhite ) + 8;

	why = SM_CantDo( m );
	if ( !( m->flags & SM_INFO ) )
		y += SM_Para( x, y, w, why ? why : "You can do it now", why ? smYellow : smGreen ) + 6;

	if ( m->flags & SM_INFO )
		return;
	keys = SM_Keys( m );
	y = SM_TOP + SM_LIST_H - 2 * SM_CTRL_H - 10;
	SM_Text( x, y, w, keys[0] ? va( "Played by: %s", keys ) : "No key plays it", keys[0] ? smWhite : smDim, sm.font, SM_TEXT_SCALE, qfalse );
	y += SM_CTRL_H + 4;
	if ( SM_Button( x, y, 96, sm.binding ? "Press a key..." : "Bind a key", sm.binding, qtrue ) )
		sm.binding = qtrue;
	if ( SM_Button( x + 100, y, 56, "Unbind", qfalse, (qboolean)( keys[0] != '\0' ) ) )
		SM_Unbind( m );
	if ( SM_Button( x + 160, y, w - 160, "Try it", qfalse, (qboolean)( why == NULL ) ) ) {
		SM_Close();
		SM_Play( m );
	}
}

void CL_SpecialMoves_Draw( void ) {
	int list[SM_NUM_MOVES], n, visible;
	float y;

	if ( !sm.open )
		return;
	SM_Box( SM_X, SM_Y, SM_W, SM_H, smPanel );
	SM_Text( SM_CAT_X, SM_Y + 8, 300, "SPECIAL MOVES", smOrange, sm.titleFont, SM_ITEM_SCALE + 0.2f, qfalse );
	SM_DrawState();
	if ( SM_Button( SM_X + SM_W - 12 - 56, SM_Y + 10, 56, "Close", qfalse, qtrue ) ) {
		SM_Close();
		return;
	}

	// the categories
	y = SM_TOP;
	for ( int c = 0; c < SM_NUM_CATS; c++ ) {
		const qboolean on = (qboolean)( c == sm.cat ), hover = SM_InRect( SM_CAT_X, y, SM_CAT_W, SM_ROW_H );

		SM_Box( SM_CAT_X, y, SM_CAT_W, SM_ROW_H, on ? smItemOn : hover ? smItemHover : smItem );
		SM_Text( SM_CAT_X + 6, y + 3, SM_CAT_W - 12, smCatNames[c], on || hover ? smWhite : smOrange, sm.titleFont, SM_ITEM_SCALE, qfalse );
		if ( SM_Clicked( SM_CAT_X, y, SM_CAT_W, SM_ROW_H ) && !on )
			SM_SetCat( c );
		y += SM_ROW_H + 4;
	}

	// the category's moves
	n = SM_Shown( list );
	visible = (int)( SM_LIST_H / SM_ROW_H );
	sm.scroll = Com_Clampi( 0, Q_max( 0, n - visible ), sm.scroll );
	{
		const qboolean bar = WG_Scrollbar( SM_LIST_X + SM_LIST_W - WG_SCROLL_W, SM_TOP, SM_LIST_H, n, visible, &sm.scroll,
			sm.cursorX, sm.cursorY, &sm.click, sm.clickX, sm.clickY );
		const float w = SM_LIST_W - ( bar ? WG_SCROLL_W + 2 : 0 );

		y = SM_TOP;
		for ( int i = sm.scroll; i < n && i < sm.scroll + visible; i++ ) {
			const smMove_t *m = &smMoves[list[i]];
			const qboolean on = (qboolean)( list[i] == sm.pick ), hover = SM_InRect( SM_LIST_X, y, w, SM_ROW_H );
			const qboolean can = (qboolean)( ( m->flags & SM_INFO ) || !SM_CantDo( m ) );

			SM_Box( SM_LIST_X, y, w, SM_ROW_H - 2, on ? smItemOn : hover ? smItemHover : smItem );
			SM_Text( SM_LIST_X + 6, y + 2, w - 12, m->title, on || hover ? smWhite : can ? smOrange : smDim, sm.titleFont, SM_ITEM_SCALE, qfalse );
			if ( SM_Clicked( SM_LIST_X, y, w, SM_ROW_H - 2 ) && !on ) {
				sm.pick = list[i];
				sm.binding = qfalse;
			}
			y += SM_ROW_H;
		}
	}

	SM_DrawDetail();

	if ( sm.message[0] && cls.realtime - sm.messageTime < SM_MESSAGE_MS )
		SM_Text( SM_CAT_X, SM_Y + SM_H - 20, SM_W - 24, sm.message, smWhite, sm.font, SM_TEXT_SCALE, qfalse );
	else
		SM_Text( SM_CAT_X, SM_Y + SM_H - 20, SM_W - 24, "Grey moves can't be done with what you have now. A bound key plays the move's keys for you.",
			smDim, sm.font, SM_TEXT_SCALE, qfalse );
	sm.click = qfalse;
	WG_DrawOverlay();
	re->SetColor( NULL );
	re->DrawStretchPic( sm.cursorX, sm.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

void CL_SpecialMoves_Init( void ) {
	memset( &sm, 0, sizeof( sm ) );
	memset( &sp, 0, sizeof( sp ) );
	sm.pick = -1;
	cl_specialMovesDebug = Cvar_Get( "cl_specialMovesDebug", "0", 0, "Prints each step of a special move played, and what the game shows" );
	Cmd_AddCommand( SM_CMD, SM_Cmd_f, "The saber's special moves explained, and keys bound to them" );
	Cmd_AddCommand( SM_PLAY_CMD, SM_Play_f, "Plays a special move: its keys pressed for you for a moment" );
}

void CL_SpecialMoves_Shutdown( void ) {
	SM_Close();
	SM_Stop();
}
