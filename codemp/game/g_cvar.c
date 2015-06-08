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

/*
static void CVU_YDFA(void) {
	g_tweakYellowDFA.integer ?
		(jcinfo.integer |= JAPRO_CINFO_YELLOWDFA) : (jcinfo.integer &= ~JAPRO_CINFO_YELLOWDFA);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}
*/

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

/*
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

static void CVU_EasyBackslash(void) {
	g_easyBackslash.integer ?
		(jcinfo.integer |= JAPRO_CINFO_EASYBACKSLASH) : (jcinfo.integer &= ~JAPRO_CINFO_EASYBACKSLASH);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_SpinRDFA(void) {
	g_spinRedDFA.integer ?
		(jcinfo.integer |= JAPRO_CINFO_REDDFA) : (jcinfo.integer &= ~JAPRO_CINFO_REDDFA);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}
*/

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

void CVU_TweakWeapons(void) {
	(g_tweakWeapons.integer & PSEUDORANDOM_FIRE) ?
		(jcinfo.integer |= JAPRO_CINFO_PSEUDORANDOM_FIRE) : (jcinfo.integer &= ~JAPRO_CINFO_PSEUDORANDOM_FIRE);
	(g_tweakWeapons.integer & STUN_LG) ?
		(jcinfo.integer |= JAPRO_CINFO_LG) : (jcinfo.integer &= ~JAPRO_CINFO_LG);
	(g_tweakWeapons.integer & STUN_SHOCKLANCE) ?
		(jcinfo.integer |= JAPRO_CINFO_SHOCKLANCE) : (jcinfo.integer &= ~JAPRO_CINFO_SHOCKLANCE);
	(g_tweakWeapons.integer & ALLOW_GUNROLL) ?
		(jcinfo.integer |= JAPRO_CINFO_GUNROLL) : (jcinfo.integer &= ~JAPRO_CINFO_GUNROLL);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

void CVU_TweakSaber(void) {
	(g_tweakSaber.integer & ST_ALLOW_ROLLCANCEL) ?
		(jcinfo.integer |= JAPRO_CINFO_ROLLCANCEL) : (jcinfo.integer &= ~JAPRO_CINFO_ROLLCANCEL);
	(g_tweakSaber.integer & ST_NO_REDCHAIN) ?
		(jcinfo.integer |= JAPRO_CINFO_NOREDCHAIN) : (jcinfo.integer &= ~JAPRO_CINFO_NOREDCHAIN);
	(g_tweakSaber.integer & ST_SPINREDDFA) ?
		(jcinfo.integer |= JAPRO_CINFO_REDDFA) : (jcinfo.integer &= ~JAPRO_CINFO_REDDFA);
	(g_tweakSaber.integer & ST_EASYBACKSLASH) ?
		(jcinfo.integer |= JAPRO_CINFO_EASYBACKSLASH) : (jcinfo.integer &= ~JAPRO_CINFO_EASYBACKSLASH);
	(g_tweakSaber.integer & ST_SPINBACKSLASH) ?
		(jcinfo.integer |= JAPRO_CINFO_BACKSLASH) : (jcinfo.integer &= ~JAPRO_CINFO_BACKSLASH);
	(g_tweakSaber.integer & ST_FIXYELLOWDFA) ?
		(jcinfo.integer |= JAPRO_CINFO_YELLOWDFA) : (jcinfo.integer &= ~JAPRO_CINFO_YELLOWDFA);
	(g_tweakSaber.integer & ST_JK2LUNGE) ?
		(jcinfo.integer |= JAPRO_CINFO_JK2LUNGE) : (jcinfo.integer &= ~JAPRO_CINFO_JK2LUNGE);
	(g_tweakSaber.integer & ST_JK2RDFA) ?
		(jcinfo.integer |= JAPRO_CINFO_JK2DFA) : (jcinfo.integer &= ~JAPRO_CINFO_JK2DFA);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

void CVU_TweakForce(void) {
	(g_tweakForce.integer & FT_FORCECOMBO) ?
		(jcinfo.integer |= JAPRO_CINFO_FORCECOMBO) : (jcinfo.integer &= ~JAPRO_CINFO_FORCECOMBO);
	(g_tweakForce.integer & FT_FASTGRIP) ?
		(jcinfo.integer |= JAPRO_CINFO_FASTGRIP) : (jcinfo.integer &= ~JAPRO_CINFO_FASTGRIP);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_LegDangle(void) {
	!g_LegDangle.integer ?
		(jcinfo.integer |= JAPRO_CINFO_LEGDANGLE) : (jcinfo.integer &= ~JAPRO_CINFO_LEGDANGLE);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void CVU_Jawarun(void) {
	(g_emotesDisable.integer & (1 << E_JAWARUN)) ?
		(jcinfo.integer |= JAPRO_CINFO_NOJAWARUN) : (jcinfo.integer &= ~JAPRO_CINFO_NOJAWARUN);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}	

static void CVU_HighFPS(void) {
	g_fixHighFPSAbuse.integer ?
		(jcinfo.integer |= JAPRO_CINFO_HIGHFPSFIX) : (jcinfo.integer &= ~JAPRO_CINFO_HIGHFPSFIX);
	trap->Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

static void RemoveRabbit(void) {
	int i;
	gclient_t	*cl;
	gentity_t	*ent;

	Team_ReturnFlag(TEAM_FREE);

	for (i=0;  i<level.numPlayingClients; i++) {
		cl = &level.clients[level.sortedClients[i]];

		cl->ps.powerups[PW_NEUTRALFLAG] = 0;
	}

	for (i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (ent->inuse && (ent->s.eType == ET_ITEM) && (ent->item->giTag == PW_NEUTRALFLAG) && (ent->item->giType = IT_TEAM)) {
			G_FreeEntity( ent );
			return;
		}
	}
}

static void RemoveCTFFlags(void) {
	int i;
	gclient_t	*cl;
	gentity_t	*ent;

	Team_ReturnFlag(TEAM_RED);
	Team_ReturnFlag(TEAM_BLUE);

	for (i=0;  i<level.numPlayingClients; i++) {
		cl = &level.clients[level.sortedClients[i]];

		cl->ps.powerups[PW_REDFLAG] = 0;
		cl->ps.powerups[PW_BLUEFLAG] = 0;
	}

	for (i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (ent->inuse && (ent->s.eType == ET_ITEM) && ((ent->item->giTag == PW_REDFLAG) || (ent->item->giTag == PW_BLUEFLAG)) && (ent->item->giType = IT_TEAM)) {
			G_FreeEntity( ent );
			//return;
		}
	}
}

qboolean G_FreeAmmoEntity( gitem_t *item );
static void RemoveWeaponsFromMap(void) {
	int i;
	gentity_t	*ent;
	int wDisable = g_weaponDisable.integer;

	for (i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];

		if (ent->inuse && ent->item) {
			if ((ent->item->giType == IT_WEAPON) && wDisable & (1 << ent->item->giTag)) {
				ent->think = 0;
				ent->nextthink = 0;
				ent->s.eFlags |= EF_NODRAW;
				//ent->s.eFlags |= EF_DROPPEDWEAPON; //sad hack
				ent->r.svFlags |= SVF_NOCLIENT;
				ent->r.contents = 0;
				//ent->inuse = qfalse;
			}
			else if (ent->item->giType == IT_AMMO && wDisable && G_FreeAmmoEntity(ent->item)) {
				ent->think = 0;
				ent->nextthink = 0;
				ent->s.eFlags |= EF_NODRAW;
				//ent->s.eFlags |= EF_DROPPEDWEAPON; //sad hack
				ent->r.svFlags |= SVF_NOCLIENT;
				ent->r.contents = 0;
				//ent->inuse = qfalse;
			}
		}
	}
}

qboolean G_CallSpawn( gentity_t *ent );
//qboolean G_ParseSpawnVars( qboolean inSubBSP );
static void SpawnWeaponsInMap(void) {
	int i;
	gentity_t	*ent;
	int wDisable = g_weaponDisable.integer;

	for (i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];

		if (ent->inuse && ent->item) { //eh?
			if ((ent->item->giType == IT_WEAPON) && !(wDisable & (1 << ent->item->giTag))) {
				ent->think = FinishSpawningItem;
				ent->nextthink = level.time + FRAMETIME * 2;
				ent->s.eFlags &= ~EF_NODRAW;
				//ent->s.eFlags &= ~EF_DROPPEDWEAPON; //sad hack
				ent->r.svFlags &= ~SVF_NOCLIENT;
				//ent->r.contents = CONTENTS_TRIGGER;
				//ent->inuse = qtrue;
				//trap->Print("spawning wep\n");
				//ent->spawnedBefore = qfalse;
			}
			else if (ent->item->giType == IT_AMMO && !(wDisable && G_FreeAmmoEntity(ent->item))) {
				ent->think = FinishSpawningItem;
				ent->nextthink = level.time + FRAMETIME * 2;
				ent->s.eFlags &= ~EF_NODRAW;
				//ent->s.eFlags &= ~EF_DROPPEDWEAPON; //sad hack
				ent->r.svFlags &= ~SVF_NOCLIENT;
				//ent->r.contents = CONTENTS_TRIGGER;
				//ent->inuse = qtrue;
			}
		}
	}
}

static void RemoveWeaponsFromPlayer(gentity_t *ent) {
	int disallowedWeaps = g_weaponDisable.integer & ~g_startingWeapons.integer;

	ent->client->ps.stats[STAT_WEAPONS] &= ~disallowedWeaps; //Subtract disallowed weapons from current weapons.

	if (ent->client->ps.stats[STAT_WEAPONS] <= 0)
		ent->client->ps.stats[STAT_WEAPONS] = WP_MELEE;

	if (!(ent->client->ps.stats[STAT_WEAPONS] & (1 >> ent->client->ps.weapon))) { //If our weapon selected does not appear in our weapons list
		ent->client->ps.weapon = WP_MELEE; //who knows why this does shit even if our current weapon is fine.
	}
}

void CVU_WeaponDisable( void ) {
	int i;
	gentity_t *ent;

	for (i=0 ; i < level.numConnectedClients ; i++) { //For each player, call removeweapons, and addweapons.
		ent = &g_entities[level.sortedClients[i]];
		if (ent->inuse && ent->client && !ent->client->sess.raceMode) {
			RemoveWeaponsFromPlayer(ent);
		}
	}
	RemoveWeaponsFromMap();
	SpawnWeaponsInMap();
}

void CVU_ForceDisable( void ) {
	int i;
	gentity_t *ent;

	for (i=0 ; i < level.numConnectedClients ; i++) {
		ent = &g_entities[level.sortedClients[i]];
		if (ent->inuse && ent->client && !ent->client->sess.raceMode) {
			WP_InitForcePowers( ent );
		}
	}
}

void GiveClientWeapons(gclient_t *client);
void CVU_StartingWeapons( void ) {
	int i;
	gentity_t *ent;

	for (i=0 ; i < level.numConnectedClients ; i++) { //For each player, call removeweapons, and addweapons.
		ent = &g_entities[level.sortedClients[i]];
		if (ent->inuse && ent->client && !ent->client->sess.raceMode) {
			RemoveWeaponsFromPlayer(ent);
			GiveClientWeapons(ent->client);
		}
	}
}

//Need to do all this shit again for items, but lazy.
void GiveClientItems(gclient_t *client);
void CVU_StartingItems( void ) {
	int i;
	gentity_t *ent;

	for (i=0 ; i < level.numConnectedClients ; i++) { //For each player, call removeweapons, and addweapons.
		ent = &g_entities[level.sortedClients[i]];
		if (ent->inuse && ent->client && !ent->client->sess.raceMode) {
			//RemoveItemsFromPlayer(ent);
			//Problem here is that thers no g_itemDisable cmd, so we have to parse all the other item individual disable cmds...
			//Maybe just let them keep their old items until they die, oh well.
			GiveClientItems(ent->client);
		}
	}
}

void CVU_Rabbit( void ) {
	if (g_rabbit.integer) { //
		gentity_t	*ent;

		RemoveRabbit(); //Delete the current flag first

		if (level.neutralFlag && (level.gametype == GT_FFA || level.gametype == GT_TEAM)) {
			ent = G_Spawn(qtrue);

			ent->classname = "team_CTF_neutralflag";
			VectorCopy(level.neutralFlagOrigin, ent->s.origin);

			if (!G_CallSpawn(ent))
				G_FreeEntity( ent );
		}
	}
	else {
		RemoveRabbit();
	}
}

void SetGametypeFuncSolids( void );
void G_CacheGametype( void );
void Svcmd_ResetScores_f (void);
void CVU_Gametype (void) {
	if (g_forceGametype.integer < GT_FFA || g_forceGametype.integer >= GT_MAX_GAME_TYPE)
		return;
	if (g_forceGametype.integer == GT_SIEGE || g_forceGametype.integer == GT_SINGLE_PLAYER) //Also dont even bother with siege, idk
		return;

	trap->Cvar_Register(&g_gametype, "g_gametype", "0", CVAR_SERVERINFO);
	trap->Cvar_Set( "g_gametype", va( "%i", g_forceGametype.integer ) );
	trap->Cvar_Update( &g_gametype );

	G_CacheGametype();
	Svcmd_ResetScores_f();

	if (level.gametype == GT_CTF || level.gametype == GT_CTY) { //
		gentity_t	*ent;

		RemoveCTFFlags(); //Delete the current flag first

		if (level.redFlag) {
			ent = G_Spawn(qtrue);
			ent->classname = "team_CTF_redflag";
			VectorCopy(level.redFlagOrigin, ent->s.origin);
			if (!G_CallSpawn(ent))
				G_FreeEntity( ent );
		}
		if (level.blueFlag) {
			ent = G_Spawn(qtrue);
			ent->classname = "team_CTF_blueflag";
			VectorCopy(level.blueFlagOrigin, ent->s.origin);
			if (!G_CallSpawn(ent))
				G_FreeEntity( ent );
		}
	}
	else {
		RemoveCTFFlags();
	}

	SetGametypeFuncSolids();

	//Spawn / clear ctf flags?  
	//who knows what needs to be done for siege.. forget it.

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
