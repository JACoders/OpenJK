#include "g_local.h"

//
// Cvar callbacks
//

//[JAPRO - Serverside - jcinfo update]
static void CVU_Flipkick(void) {
	if (g_flipKick.integer > 2) {//3
		jcinfo.integer |= JAPRO_CINFO_FLIPKICK;
		jcinfo.integer |= JAPRO_CINFO_FIXSIDEKICK;
	}
	else if (g_flipKick.integer == 2) {//1
		jcinfo.integer |= JAPRO_CINFO_FLIPKICK;
		jcinfo.integer &= ~JAPRO_CINFO_FIXSIDEKICK;
	}
	else if (g_flipKick.integer == 1) {//1
		jcinfo.integer |= JAPRO_CINFO_FLIPKICK;
		jcinfo.integer &= ~JAPRO_CINFO_FIXSIDEKICK;
	}
	else {//0
		jcinfo.integer &= ~JAPRO_CINFO_FLIPKICK;
		jcinfo.integer &= ~JAPRO_CINFO_FIXSIDEKICK;
	}
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_Roll(void) {
	if (g_fixRoll.integer > 2) {
		jcinfo.integer |= JAPRO_CINFO_FIXROLL3;
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL2;
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL1;
	}
	else if (g_fixRoll.integer == 2) {
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL3;
		jcinfo.integer |= JAPRO_CINFO_FIXROLL2;
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL1;
	}
	else if (g_fixRoll.integer == 1) {
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL3;
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL2;
		jcinfo.integer |= JAPRO_CINFO_FIXROLL1;
	}
	else {
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL3;
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL2;
		jcinfo.integer &= ~JAPRO_CINFO_FIXROLL1;
	}
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_YDFA(void) {
	g_tweakYellowDFA.integer ?
		(jcinfo.integer |= JAPRO_CINFO_YELLOWDFA) : (jcinfo.integer &= ~JAPRO_CINFO_YELLOWDFA);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_Headslide(void) {
	g_slideOnPlayer.integer ?
		(jcinfo.integer |= JAPRO_CINFO_HEADSLIDE) : (jcinfo.integer &= ~JAPRO_CINFO_HEADSLIDE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_Unlagged(void) {
	if (g_unlagged.integer & UNLAGGED_PROJ_NUDGE)//if 1 or 3 or 7
		jcinfo.integer |= JAPRO_CINFO_UNLAGGEDPROJ;
	else
		jcinfo.integer &= ~JAPRO_CINFO_UNLAGGEDPROJ;
	if (g_unlagged.integer & UNLAGGED_HITSCAN)//if 2 or 6?
		jcinfo.integer |= JAPRO_CINFO_UNLAGGEDHITSCAN;
	else
		jcinfo.integer &= ~JAPRO_CINFO_UNLAGGEDHITSCAN;
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_Bhop(void) {
	if (g_onlyBhop.integer > 1) {
		jcinfo.integer &= ~JAPRO_CINFO_BHOP1;
		jcinfo.integer |= JAPRO_CINFO_BHOP2;
	}
	else if (g_onlyBhop.integer == 1) {
		jcinfo.integer |= JAPRO_CINFO_BHOP1;
		jcinfo.integer &= ~JAPRO_CINFO_BHOP2;
	}
	else {
		jcinfo.integer &= ~JAPRO_CINFO_BHOP1;
		jcinfo.integer &= ~JAPRO_CINFO_BHOP2;
	}
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_FastGrip(void) {
	g_fastGrip.integer ?
		(jcinfo.integer |= JAPRO_CINFO_FASTGRIP) : (jcinfo.integer &= ~JAPRO_CINFO_FASTGRIP);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_SpinBackslash(void) {
	g_spinBackslash.integer ?
		(jcinfo.integer |= JAPRO_CINFO_BACKSLASH) : (jcinfo.integer &= ~JAPRO_CINFO_BACKSLASH);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_SpinRDFA(void) {
	g_spinRedDFA.integer ?
		(jcinfo.integer |= JAPRO_CINFO_REDDFA) : (jcinfo.integer &= ~JAPRO_CINFO_REDDFA);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_TweakJetpack(void) {
	g_tweakJetpack.integer ?
		(jcinfo.integer |= JAPRO_CINFO_JETPACK) : (jcinfo.integer &= ~JAPRO_CINFO_JETPACK);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_ScreenShake(void) {
	g_screenShake.integer ?
		(jcinfo.integer |= JAPRO_CINFO_SCREENSHAKE) : (jcinfo.integer &= ~JAPRO_CINFO_SCREENSHAKE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_ForceCombo(void) { //Only needed to predict speed+darkrage runspeed :/
	g_forceCombo.integer ?
		(jcinfo.integer |= JAPRO_CINFO_FORCECOMBO) : (jcinfo.integer &= ~JAPRO_CINFO_FORCECOMBO);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_TweakWeapons(void) {
	(g_tweakWeapons.integer & PSEUDORANDOM_FIRE) ?
		(jcinfo.integer |= JAPRO_CINFO_PSEUDORANDOM_FIRE) : (jcinfo.integer &= ~JAPRO_CINFO_PSEUDORANDOM_FIRE);
	(g_tweakWeapons.integer & STUN_LG) ?
		(jcinfo.integer |= JAPRO_CINFO_LG) : (jcinfo.integer &= ~JAPRO_CINFO_LG);
	(g_tweakWeapons.integer & STUN_SHOCKLANCE) ?
		(jcinfo.integer |= JAPRO_CINFO_SHOCKLANCE) : (jcinfo.integer &= ~JAPRO_CINFO_SHOCKLANCE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_MovementStyle(void) {
	if (g_movementStyle.integer == 1) {
		jcinfo.integer &= ~JAPRO_CINFO_NOSTRAFE;
		jcinfo.integer &= ~JAPRO_CINFO_HL2;
		jcinfo.integer &= ~JAPRO_CINFO_CPM;
	}
	else if (g_movementStyle.integer == 2) {
		jcinfo.integer |= JAPRO_CINFO_HL2;
		jcinfo.integer &= ~JAPRO_CINFO_CPM;
		jcinfo.integer &= ~JAPRO_CINFO_NOSTRAFE;
	}
	else if (g_movementStyle.integer > 2) {
		jcinfo.integer |= JAPRO_CINFO_CPM;
		jcinfo.integer &= ~JAPRO_CINFO_HL2;
		jcinfo.integer &= ~JAPRO_CINFO_NOSTRAFE;
	}
	else { // 0
		jcinfo.integer |= JAPRO_CINFO_NOSTRAFE;
		jcinfo.integer &= ~JAPRO_CINFO_HL2;
		jcinfo.integer &= ~JAPRO_CINFO_CPM;
	}
	trap->Cvar_Set( "jcinfo", va( "%i", jcinfo.integer ) );
}

static void CVU_LegDangle(void) {
	g_LegDangle.integer ?
		(jcinfo.integer |= JAPRO_CINFO_LEGDANGLE) : (jcinfo.integer &= ~JAPRO_CINFO_LEGDANGLE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_JK2Lunge(void) {
	g_jk2Lunge.integer ?
		(jcinfo.integer |= JAPRO_CINFO_JK2LUNGE) : (jcinfo.integer &= ~JAPRO_CINFO_JK2LUNGE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_JK2DFA(void) {
	g_jk2DFA.integer ?
		(jcinfo.integer |= JAPRO_CINFO_JK2DFA) : (jcinfo.integer &= ~JAPRO_CINFO_JK2DFA);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_Jawarun(void) {
	(g_emotesDisable.integer & (1 << E_JAWARUN)) ?
		(jcinfo.integer |= JAPRO_CINFO_NOJAWARUN) : (jcinfo.integer &= ~JAPRO_CINFO_NOJAWARUN);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}	

static void CVU_Dodge(void) {
	if (g_dodge.integer == 1) {
		jcinfo.integer |= JAPRO_CINFO_DODGE;
		jcinfo.integer &= ~JAPRO_CINFO_DASH;
	}
	else if (g_dodge.integer > 1) {
		jcinfo.integer |= JAPRO_CINFO_DODGE;
		jcinfo.integer |= JAPRO_CINFO_DASH;
	}
	else {
		jcinfo.integer &= ~JAPRO_CINFO_DODGE;
		jcinfo.integer &= ~JAPRO_CINFO_DASH;
	}
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_RampJump(void) {
	g_rampJump.integer ?
		(jcinfo.integer |= JAPRO_CINFO_RAMPJUMP) : (jcinfo.integer &= ~JAPRO_CINFO_RAMPJUMP);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_OverBounce(void) {
	g_overBounce.integer ?
		(jcinfo.integer |= JAPRO_CINFO_OVERBOUNCE) : (jcinfo.integer &= ~JAPRO_CINFO_OVERBOUNCE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}


//
// Cvar table
//

typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	int			cvarFlags;
	qboolean	trackChange; // announce if value changes
} cvarTable_t;

#define XCVAR_DECL
	#include "g_xcvar.h"
#undef XCVAR_DECL

static const cvarTable_t gameCvarTable[] = {
	#define XCVAR_LIST
		#include "g_xcvar.h"
	#undef XCVAR_LIST
};
static const size_t gameCvarTableSize = ARRAY_LEN( gameCvarTable );

void G_RegisterCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=gameCvarTable; i<gameCvarTableSize; i++, cv++ ) {
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

void G_UpdateCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=gameCvarTable; i<gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount != modCount ) {
				if ( cv->update )
					cv->update();

				if ( cv->trackChange )
					trap->SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string ) );
			}
		}
	}
}
