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

// cl_emotepreview.cpp -- the emotes' animations, shown in RPMod's Help menu
//
// RPMod's Help, Emote Commands, lists the emotes, and a click on one sends
// rphelp "<emote>" uipt. Beside the list the emote is shown playing on the player's own
// model, the way the single player datapad shows its moves.
//
// Which animation an emote plays is the server's to know: nothing says it, the character
// just plays it. So each emote the player does is watched, its animation read off the
// next snapshots and kept in emoteanims.txt; one not done yet can't be shown. RPMod's own,
// learned once, come built in: what's learned here goes over them.
//
//   emotepreview             the emotes learned and their animations
//   emotepreview forget <e>  one forgotten, to learn again
//   emotepreview watch       every change to the player's animation printed, after an emote

#include "client.h"
#include "ghoul2/G2.h"
#include "cgame/animtable.h"

#define EP_FILE				"emoteanims.txt"
#define EP_MAX				512
#define EP_NAME_LEN			48
#define EP_LEARN_MS			2000	// the animation has to change this soon after the emote goes
#define EP_WATCH_MS			4000	// watch prints changes this long after an emote
#define EP_HOLD_MS			1500	// one that doesn't loop holds its last pose this long, then starts over
#define EP_BOX_X			436.0f	// in 640x480: right of the Help menu's list
#define EP_BOX_Y			112.0f
#define EP_BOX_W			156.0f
#define EP_BOX_H			236.0f
#define EP_TEXT_SCALE		0.7f

// RPMod's animations are the game's with seven put in after LEGS_TURN180: what the snapshot's
// numbers are from there on are the game's seven further on
#define EP_RPMOD_FIRST		( LEGS_TURN180 + 1 )
static const char *epRpmodAnims[] = {
	"BOTH_LEDGE_GRAB", "BOTH_LEDGE_HOLD", "BOTH_LEDGE_LEFT", "BOTH_LEDGE_RIGHT",
	"BOTH_LEDGE_MERCPULL", "BOTH_RAISEHAND", "BOTH_LOWERHAND"
};

typedef struct {
	char		emote[EP_NAME_LEN];
	char		torso[EP_NAME_LEN];
	char		legs[EP_NAME_LEN];
	qboolean	builtIn;		// from the table below, not learned here: not saved
} epEmote_t;

// RPMod's emotes, learned on Temple: emote, torso, and legs when they differ
static const char *epBuiltIn[][3] = {
	{ "emtalk", "BOTH_STAND1_TALK1", NULL },
	{ "emtalk2", "BOTH_TALK1", NULL },
	{ "emcomm", "BOTH_TALKCOMM1START", NULL },
	{ "ematease2", "BOTH_CIN_10", NULL },
	{ "emknockdown", "BOTH_KNOCKDOWN1", NULL },
	{ "emknockdown2", "BOTH_KNOCKDOWN2", NULL },
	{ "emknockdown3", "BOTH_KNOCKDOWN3", NULL },
	{ "emkneel", "BOTH_CROUCH3", NULL },
	{ "emkneel2", "BOTH_FORCEHEAL_START", NULL },
	{ "emkneel3", "BOTH_ROSH_PAIN", NULL },
	{ "emkneel4", "BOTH_CIN_19", NULL },
	{ "emsurrender", "TORSO_SURRENDER_START", NULL },
	{ "emraisehand", "BOTH_RAISEHAND", NULL },
	{ "emsleep", "BOTH_SLEEP1", NULL },
	{ "emconsole", "BOTH_CONSOLE1START", NULL },
	{ "emconsolecomm", "BOTH_CONSOLE2START", NULL },
	{ "emyes", "BOTH_HEADNOD", NULL },
	{ "emno", "BOTH_HEADSHAKE", NULL },
	{ "emcomeon", "BOTH_FORCEGRIP3THROW", NULL },
	{ "emcomeon2", "BOTH_COME_ON1", NULL },
	{ "emhello", "BOTH_SILENCEGESTURE1", NULL },
	{ "embeg", "BOTH_KNEES2", NULL },
	{ "emcower", "BOTH_COWER1_START", NULL },
	{ "emnoisy", "BOTH_SONICPAIN_START", NULL },
	{ "emwait", "BOTH_STAND10", NULL },
	{ "ematease", "BOTH_STAND4", NULL },
	{ "emhips", "BOTH_STAND5TOSTAND8", NULL },
	{ "emreach", "BOTH_REACH1START", NULL },
	{ "emreach2", "BOTH_REACHFORSABER1", NULL },
	{ "emlotus", "BOTH_STAND5TOSIT2", NULL },
	{ "emsit", "BOTH_STAND5TOSIT3", NULL },
	{ "emsit2", "BOTH_CIN_28", NULL },
	{ "emsit3", "BOTH_CIN_33", NULL },
	{ "emsit4", "BOTH_CIN_14", NULL },
	{ "emsit5", "BOTH_CIN_15", NULL },
	{ "emsit6", "BOTH_CIN_16", NULL },
	{ "emsit7", "BOTH_CIN_13", NULL },
	{ "emsit8", "BOTH_CIN_30", NULL },
	{ "emsit9", "BOTH_CIN_29", NULL },
	{ "emsit10", "BOTH_CIN_31", NULL },
	{ "emsit11", "BOTH_CIN_32", NULL },
	{ "emsit12", "BOTH_SIT5", NULL },
	{ "emsitconsole", "BOTH_CIN_34", NULL },
	{ "emliedown", "BOTH_SIT6", NULL },
	{ "emhandsignal1", "TORSO_HANDSIGNAL1", NULL },
	{ "emhandsignal2", "TORSO_HANDSIGNAL2", NULL },
	{ "emhandsignal3", "TORSO_HANDSIGNAL3", NULL },
	{ "emhandsignal4", "TORSO_HANDSIGNAL4", NULL },
	{ "emhandsignall", "BOTH_TOSS1", NULL },
	{ "emhandsignalr", "BOTH_TOSS2", NULL },
	{ "emgrip", "BOTH_FORCEGRIP_HOLD", NULL },
	{ "emgrip2", "BOTH_FORCELIGHTNING_START", NULL },
	{ "emgrip3", "BOTH_FORCE_2HANDEDLIGHTNING_START", NULL },
	{ "emgrip4", "BOTH_SABERPULL", NULL },
	{ "emthrow", "BOTH_ATTACK10", NULL },
	{ "embutton", "BOTH_ATTACK11", NULL },
	{ "embutton2", "BOTH_BUTTON_HOLD", NULL },
	{ "emsniper", "TORSO_WEAPONIDLE4", NULL },
	{ "emknockback", "BOTH_BASHED1", NULL },
	{ "emhurt", "BOTH_CHOKE2", NULL },
	{ "emgrab", "BOTH_FORCE_DRAIN_GRAB_START", NULL },
	{ "emgrabbed", "BOTH_FORCE_DRAIN_GRABBED", NULL },
	{ "emheadtiltl", "BOTH_HEADTILTLSTART", NULL },
	{ "emshy", "BOTH_SABERKILLEE1", NULL },
	{ "emshuffle", "BOTH_STAND5IDLE1", NULL },
	{ "embalance", "BOTH_STEADYSELF1", NULL },
	{ "emdeath", "BOTH_DEATH10", NULL },
	{ "emdeath2", "BOTH_DEATH13", NULL },
	{ "emdeath3", "BOTH_DEATH18", NULL },
	{ "emdeath4", "BOTH_DEATH2", NULL },
	{ "emdead", "BOTH_DEAD21", NULL },
	{ "emdead2", "BOTH_DEAD22", NULL },
	{ "emdead3", "BOTH_DEAD19", NULL },
	{ "emdodgel", "BOTH_DODGE_L", NULL },
	{ "emdodger", "BOTH_DODGE_R", NULL },
	{ "embackflip", "BOTH_JUMPATTACK7", NULL },
	{ "emsaberkick", "BOTH_LK_ST_DL_T_SB_1_W", NULL },
	{ "emsaberpose", "BOTH_P7_S7_BR", NULL },
	{ "emsaberpose2", "BOTH_P7_S7_T_", NULL },
	{ "emsaberpose3", "BOTH_CIN_39", NULL },
	{ "emsaberpose4", "BOTH_CIN_40", NULL },
	{ "emsaberpose5", "BOTH_CIN_41", NULL },
	{ "emsaberpose6", "BOTH_CIN_42", NULL },
	{ "emsaberfinish", "BOTH_SABERKILLER1", NULL },
	{ "emsaberthrow", "BOTH_SABERTHROW1START", NULL },
	{ "emsaberthrow2", "BOTH_SABERTHROW2START", NULL },
	{ "empoint", "BOTH_STAND5STARTLEDLOOKLEFT", NULL },
	{ "empoint2", "BOTH_STAND5TOAIM", NULL },
	{ "emwindy", "BOTH_WIND", NULL },
	{ "emsabertaunt", "BOTH_LK_ST_S_S_SB_1_W", NULL },
	{ "emsabertaunt2", "BOTH_A6_FB", NULL },
	{ "emsabertaunt3", "BOTH_A6_LR", NULL },
	{ "emcheer", "BOTH_TUSKENTAUNT1", NULL },
	{ "emsabertaunt4", "BOTH_A6_SABERPROTECT", NULL },
	{ "emsaberlock1", "BOTH_BF1LOCK", NULL },
	{ "emsaberlock2", "BOTH_BF2LOCK", NULL },
	{ "emcrossarms", "BOTH_CIN_4", NULL },
	{ "emlift", "BOTH_CIN_26", NULL },
	{ "emlift2", "BOTH_CIN_27", NULL },
	{ "empose", "BOTH_CIN_11", NULL },
	{ "empose2", "BOTH_CIN_35", NULL },
	{ "empose3", "BOTH_CIN_36", NULL },
	{ "empose4", "BOTH_CIN_37", NULL },
	{ "emsalute", "BOTH_CIN_38", NULL },
	{ "emlean", "BOTH_CIN_7", NULL },
	{ "emgun", "BOTH_CIN_1", NULL },
};

static epEmote_t	epEmotes[EP_MAX];
static int			epNumEmotes;
static qboolean		epLoaded;

static struct {
	// watch: the last emote, and the animation printed last
	qboolean	watch;
	char		watched[EP_NAME_LEN];
	int			watchTime, watchTorso, watchLegs;
	qboolean	watchTorsoFlip, watchLegsFlip;

	// one just done, its animation awaited
	char		learning[EP_NAME_LEN];
	int			learnTime;
	int			torso, legs;
	qboolean	torsoFlip, legsFlip;

	// the one clicked in the Help menu, and the model playing it
	char		shown[EP_NAME_LEN];
	CGhoul2Info_v *g2;
	char		g2Key[MAX_QPATH * 2 + EP_NAME_LEN];	// model and emote it was built for
	vec3_t		mins, maxs;
	int			animStart, cycleMs;		// one that doesn't loop starts over after cycleMs
	int			pelvis;				// the bolt the camera follows, -1 if none
	float		lookZ;				// where it looks, eased after the pelvis
	const char	*error;
	int			font;
} ep;

static const vec4_t epPanel		= { 0.0f, 0.0f, 0.05f, 0.85f };
static const vec4_t epBorder	= { 0.66f, 0.66f, 1.0f, 1.0f };
static const vec4_t epWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static const vec4_t epDim		= { 0.7f, 0.7f, 0.85f, 1.0f };

/*
===============================================================================

What's been learned

===============================================================================
*/

// a snapshot's animation number by name, RPMod's numbering; NULL if it isn't one
static const char *EP_AnimName( int anim ) {
	if ( anim < 0 )
		return NULL;
	if ( anim >= EP_RPMOD_FIRST && anim < EP_RPMOD_FIRST + (int)ARRAY_LEN( epRpmodAnims ) )
		return epRpmodAnims[anim - EP_RPMOD_FIRST];
	if ( anim >= EP_RPMOD_FIRST )
		anim -= ARRAY_LEN( epRpmodAnims );
	if ( anim >= MAX_ANIMATIONS )
		return NULL;
	return GetStringForID( animTable, anim );
}

static epEmote_t *EP_Find( const char *emote ) {
	for ( int i = 0; i < epNumEmotes; i++ ) {
		if ( !Q_stricmp( epEmotes[i].emote, emote ) )
			return &epEmotes[i];
	}
	return NULL;
}

// the built in ones the file didn't have
static void EP_AddBuiltIn( void ) {
	for ( size_t i = 0; i < ARRAY_LEN( epBuiltIn ) && epNumEmotes < EP_MAX; i++ ) {
		epEmote_t *e;

		if ( EP_Find( epBuiltIn[i][0] ) )
			continue;
		e = &epEmotes[epNumEmotes++];
		Q_strncpyz( e->emote, epBuiltIn[i][0], EP_NAME_LEN );
		Q_strncpyz( e->torso, epBuiltIn[i][1], EP_NAME_LEN );
		Q_strncpyz( e->legs, epBuiltIn[i][2] ? epBuiltIn[i][2] : epBuiltIn[i][1], EP_NAME_LEN );
		e->builtIn = qtrue;
	}
}

static void EP_Load( void ) {
	char *buf, *line, *next;

	if ( epLoaded )
		return;
	epLoaded = qtrue;
	epNumEmotes = 0;
	if ( FS_ReadFile( EP_FILE, (void **)&buf ) <= 0 || !buf ) {
		EP_AddBuiltIn();
		return;
	}
	for ( line = buf; line && *line && epNumEmotes < EP_MAX; line = next ) {
		char emote[EP_NAME_LEN], torso[EP_NAME_LEN], legs[EP_NAME_LEN];

		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		if ( line[0] == '/' && line[1] == '/' )
			continue;
		if ( sscanf( line, "%47s %47s %47s", emote, torso, legs ) == 3 && !EP_Find( emote ) ) {
			Q_strncpyz( epEmotes[epNumEmotes].emote, emote, EP_NAME_LEN );
			Q_strncpyz( epEmotes[epNumEmotes].torso, torso, EP_NAME_LEN );
			Q_strncpyz( epEmotes[epNumEmotes].legs, legs, EP_NAME_LEN );
			epNumEmotes++;
		}
	}
	FS_FreeFile( buf );
	EP_AddBuiltIn();
}

// the emotes known, for the cinematic manager: their names and the animation each plays
int CL_EmotePreview_Count( void ) {
	EP_Load();
	return epNumEmotes;
}

const char *CL_EmotePreview_Name( int index ) {
	EP_Load();
	return index >= 0 && index < epNumEmotes ? epEmotes[index].emote : NULL;
}

// the animation an emote plays, NULL if it isn't known
const char *CL_EmotePreview_Anim( const char *emote ) {
	const epEmote_t *e;

	EP_Load();
	e = EP_Find( emote );
	return e ? e->torso : NULL;
}

static void EP_Save( void ) {
	fileHandle_t f = FS_FOpenFileWrite( EP_FILE );

	if ( !f )
		return;
	FS_Printf( f, "// RPMod emotes and the animations they play, torso then legs: learned by doing them\n" );
	for ( int i = 0; i < epNumEmotes; i++ ) {
		if ( !epEmotes[i].builtIn )
			FS_Printf( f, "%s %s %s\n", epEmotes[i].emote, epEmotes[i].torso, epEmotes[i].legs );
	}
	FS_FCloseFile( f );
}

static void EP_Learned( const char *emote, const char *torso, const char *legs ) {
	epEmote_t *e;

	EP_Load();
	e = EP_Find( emote );
	if ( e && !Q_stricmp( e->torso, torso ) && !Q_stricmp( e->legs, legs ) )
		return;
	if ( !e ) {
		if ( epNumEmotes == EP_MAX )
			return;
		e = &epEmotes[epNumEmotes++];
		Q_strncpyz( e->emote, emote, EP_NAME_LEN );
	}
	Q_strncpyz( e->torso, torso, EP_NAME_LEN );
	Q_strncpyz( e->legs, legs, EP_NAME_LEN );
	e->builtIn = qfalse;
	EP_Save();
	// the preview of it is built again
	ep.g2Key[0] = '\0';
	Com_Printf( S_COLOR_GREY "Emote %s plays %s: Help shows it from now on\n", emote, torso );
}

// an emote's name: em and letters or numbers, nothing else
static qboolean EP_IsEmote( const char *name ) {
	if ( Q_stricmpn( name, "em", 2 ) || !name[2] || strlen( name ) >= EP_NAME_LEN )
		return qfalse;
	for ( ; *name; name++ ) {
		if ( !isalnum( (unsigned char)*name ) )
			return qfalse;
	}
	return qtrue;
}

// a command on its way to the server: an emote is watched, a Help click shows one
void CL_EmotePreview_Command( const char *cmd ) {
	char word[EP_NAME_LEN], arg[EP_NAME_LEN];
	const char *p = cmd;
	int len = 0;

	while ( *p == ' ' )
		p++;
	while ( *p && *p != ' ' && len < EP_NAME_LEN - 1 )
		word[len++] = *p++;
	word[len] = '\0';

	if ( !Q_stricmp( word, "rphelp" ) ) {
		// rphelp "<command>" uipt
		while ( *p == ' ' || *p == '"' )
			p++;
		for ( len = 0; *p && *p != '"' && *p != ' ' && len < EP_NAME_LEN - 1; )
			arg[len++] = *p++;
		arg[len] = '\0';
		if ( EP_IsEmote( arg ) )
			Q_strncpyz( ep.shown, arg, sizeof( ep.shown ) );
		else
			ep.shown[0] = '\0';
		return;
	}
	if ( !EP_IsEmote( word ) || !cl.snap.valid )
		return;
	if ( ep.watch ) {
		Q_strncpyz( ep.watched, word, sizeof( ep.watched ) );
		ep.watchTime = cls.realtime;
		ep.watchTorso = cl.snap.ps.torsoAnim;
		ep.watchLegs = cl.snap.ps.legsAnim;
		ep.watchTorsoFlip = cl.snap.ps.torsoFlip;
		ep.watchLegsFlip = cl.snap.ps.legsFlip;
		Com_Printf( "%s: torso %s, legs %s before\n", word, EP_AnimName( ep.watchTorso ), EP_AnimName( ep.watchLegs ) );
	}
	Q_strncpyz( ep.learning, word, sizeof( ep.learning ) );
	ep.learnTime = cls.realtime;
	ep.torso = cl.snap.ps.torsoAnim;
	ep.legs = cl.snap.ps.legsAnim;
	ep.torsoFlip = cl.snap.ps.torsoFlip;
	ep.legsFlip = cl.snap.ps.legsFlip;
}

/*
===============================================================================

The preview

===============================================================================
*/

static void EP_Free( void ) {
	if ( ep.g2 )
		re->G2API_CleanGhoul2Models( &ep.g2 );
	ep.g2 = NULL;
	ep.g2Key[0] = '\0';
}

// one animation from the animation.cfg beside the model's .gla, on the bone; how long it plays
// in ms, 0 if it loops, -1 if it isn't there
static int EP_SetAnim( CGhoul2Info_v &g2, const char *cfg, const char *anim, const char *bone ) {
	const char *p = cfg, *token;

	COM_BeginParseSession( "emotepreview" );
	while ( 1 ) {
		int first, num, loop, flags;
		float fps;

		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
			return -1;
		if ( Q_stricmp( token, anim ) ) {
			SkipRestOfLine( &p );
			continue;
		}
		first = atoi( COM_ParseExt( &p, qfalse ) );
		num = atoi( COM_ParseExt( &p, qfalse ) );
		loop = atoi( COM_ParseExt( &p, qfalse ) );
		fps = (float)atof( COM_ParseExt( &p, qfalse ) );
		if ( num <= 0 || !fps )
			return -1;
		// as cgame: the speed is 50 / ( 1000 / fps ), and a negative one plays it backwards, from its
		// end to its start; one that doesn't loop stays on its last frame
		flags = loop == -1 ? BONE_ANIM_OVERRIDE_FREEZE : BONE_ANIM_OVERRIDE_LOOP;
		if ( fps > 0.0f )
			re->G2API_SetBoneAnim( g2, 0, bone, first, first + num, flags, fps / 20.0f, re->G2API_GetTime( cl.serverTime ), -1, -1 );
		else
			re->G2API_SetBoneAnim( g2, 0, bone, first + num, first, flags, fps / 20.0f, re->G2API_GetTime( cl.serverTime ), -1, -1 );
		return loop == -1 ? (int)( num * 1000.0f / fabsf( fps ) ) : 0;
	}
}

static void EP_Animate( const epEmote_t *e ) {
	char path[MAX_QPATH], *buf, *slash;
	int ms;
	const char *gla = re->G2API_GetGLAName( *ep.g2, 0 );

	if ( !gla || !gla[0] )
		return;
	Q_strncpyz( path, gla, sizeof( path ) );
	slash = strrchr( path, '/' );
	if ( !slash )
		return;
	slash[1] = '\0';
	Q_strcat( path, sizeof( path ), "animation.cfg" );
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf ) {
		ep.error = "The model has no animations";
		return;
	}
	// as cgame: the legs on the whole, the torso over them from the waist up
	ms = EP_SetAnim( *ep.g2, buf, e->legs, "model_root" );
	if ( ms < 0 )
		ms = EP_SetAnim( *ep.g2, buf, e->torso, "model_root" );
	if ( ms < 0 ) {
		ep.error = "Its animation isn't in this model's";
	} else if ( Q_stricmp( e->torso, e->legs ) ) {
		const int torsoMs = EP_SetAnim( *ep.g2, buf, e->torso, "lower_lumbar" );

		ms = Q_max( ms, torsoMs );
	}
	FS_FreeFile( buf );
	// played once, it holds its last pose a while and starts over
	ep.animStart = cls.realtime;
	ep.cycleMs = ms > 0 ? ms + EP_HOLD_MS : 0;
}

// a player's animations, by number as the server sends them, put on a ghoul2 instance as cgame
// does: the legs on the whole, the torso from the waist up. For a model the cgame just made, which
// would stand in its bind pose until the animation next changes
void CL_EmotePreview_PlayerAnims( void *ghoul2, int legsAnim, int torsoAnim ) {
	CGhoul2Info_v &g2 = *(CGhoul2Info_v *)ghoul2;
	const char *gla, *legs = EP_AnimName( legsAnim ), *torso = EP_AnimName( torsoAnim );
	char path[MAX_QPATH], *buf, *slash;

	if ( !ghoul2 || !re->G2API_HaveWeGhoul2Models( g2 ) )
		return;
	gla = re->G2API_GetGLAName( g2, 0 );
	if ( !gla || !gla[0] )
		return;
	Q_strncpyz( path, gla, sizeof( path ) );
	if ( !( slash = strrchr( path, '/' ) ) )
		return;
	slash[1] = '\0';
	Q_strcat( path, sizeof( path ), "animation.cfg" );
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	if ( legs )
		EP_SetAnim( g2, buf, legs, "model_root" );
	if ( torso )
		EP_SetAnim( g2, buf, torso, "lower_lumbar" );
	FS_FreeFile( buf );
}

// the player's own model, as everyone sees it
static const char *EP_PlayerModel( void ) {
	const int index = cl.gameState.stringOffsets[CS_PLAYERS + cl.snap.ps.clientNum];
	const char *model = index ? Info_ValueForKey( cl.gameState.stringData + index, "model" ) : "";

	return model[0] ? model : "kyle";
}

// (re)builds the model when the emote or the player's model changed
static void EP_Update( const epEmote_t *e ) {
	char key[sizeof( ep.g2Key )], name[MAX_QPATH], glm[MAX_QPATH], skinPath[MAX_QPATH], *slash;
	const char *model = EP_PlayerModel(), *skin = "default";
	qhandle_t hSkin;

	Com_sprintf( key, sizeof( key ), "%s|%s", model, e->emote );
	if ( !Q_stricmp( key, ep.g2Key ) )
		return;
	EP_Free();
	Q_strncpyz( ep.g2Key, key, sizeof( ep.g2Key ) );
	ep.error = NULL;

	// "model" or "model/skin"
	Q_strncpyz( name, model, sizeof( name ) );
	slash = strchr( name, '/' );
	if ( slash ) {
		*slash = '\0';
		if ( slash[1] )
			skin = slash + 1;
	}
	Com_sprintf( glm, sizeof( glm ), "models/players/%s/model.glm", name );
	ep.error = CL_GlmBounds( glm, ep.mins, ep.maxs );
	if ( ep.error )
		return;
	if ( strchr( skin, '|' ) ) {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/|%s", name, skin );
	} else {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_%s.skin", name, skin );
		if ( FS_ReadFile( skinPath, NULL ) <= 0 )
			Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_default.skin", name );
	}
	hSkin = re->RegisterSkin( skinPath );
	if ( re->G2API_InitGhoul2Model( &ep.g2, glm, 0, hSkin, 0, 0, 0 ) < 0 || !ep.g2 ) {
		EP_Free();
		Q_strncpyz( ep.g2Key, key, sizeof( ep.g2Key ) );	// not again every frame
		ep.error = "Couldn't load your model";
		return;
	}
	re->G2API_SetSkin( *ep.g2, 0, hSkin, hSkin );
	EP_Animate( e );
	ep.pelvis = re->G2API_AddBolt( *ep.g2, 0, "pelvis" );
	ep.lookZ = ( ep.mins[2] + ep.maxs[2] ) * 0.5f;
}

static void EP_Render( float x, float y, float w, float h ) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t center, forward, angles;
	const float xScale = cls.glconfig.vidWidth / (float)SCREEN_WIDTH, yScale = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;
	float fovX, fovY, halfH, radius, dist;

	memset( &ent, 0, sizeof( ent ) );
	// room for arms flung out and lying down: the model's height every way
	halfH = ( ep.maxs[2] - ep.mins[2] ) * 0.5f;
	radius = halfH;
	VectorSet( ent.angles, 0, 20.0f + sinf( cls.realtime * 0.0006f ) * 35.0f, 0 );	// turning a little either way

	// the camera follows the pelvis down when it sits, kneels or lies, a little behind it
	if ( ep.pelvis >= 0 ) {
		mdxaBone_t bolt;
		vec3_t zero = { 0, 0, 0 };

		if ( re->G2API_GetBoltMatrix( *ep.g2, 0, ep.pelvis, &bolt, ent.angles, zero, cl.serverTime, NULL, zero ) )
			ep.lookZ += ( bolt.matrix[2][3] + halfH * 0.15f - ep.lookZ ) * 0.15f;
	}
	VectorSet( center, 0, 0, ep.lookZ );

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.x = (int)( x * xScale );
	refdef.y = (int)( y * yScale );
	refdef.width = (int)( w * xScale );
	refdef.height = (int)( h * yScale );
	fovY = 30.0f;
	fovX = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovY * 0.5f ) ) * refdef.width / (float)refdef.height ) );
	refdef.fov_x = fovX;
	refdef.fov_y = fovY;
	dist = Q_max( halfH / tanf( DEG2RAD( fovY * 0.5f ) ), radius / tanf( DEG2RAD( fovX * 0.5f ) ) ) * 1.05f;

	VectorSet( angles, 10.0f, 180.0f, 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( center, -dist, forward, refdef.vieworg );
	AnglesToAxis( angles, refdef.viewaxis );
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.time = cl.serverTime;

	ent.reType = RT_MODEL;
	ent.ghoul2 = ep.g2;
	ent.renderfx = RF_NOSHADOW;
	AnglesToAxis( ent.angles, ent.axis );
	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;
	ent.radius = radius * 2.0f;

	re->ClearScene();
	re->AddRefEntityToScene( &ent );
	re->RenderScene( &refdef );
}

static void EP_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void EP_TextScaled( float x, float y, float w, float scale, const char *text, const float *color ) {
	const float tw = (float)re->Font_StrLenPixels( text, ep.font, scale );

	re->Font_DrawString( (int)( x + ( w - tw ) * 0.5f ), (int)y, text, color, ep.font | STYLE_DROPSHADOW, -1, scale );
}

static void EP_Text( float x, float y, float w, const char *text, const float *color ) {
	EP_TextScaled( x, y, w, EP_TEXT_SCALE, text, color );
}

static qboolean EP_Shown( void ) {
	return (qboolean)( ep.shown[0] && cls.state == CA_ACTIVE && cls.cgameStarted && ( Key_GetCatcher() & KEYCATCH_UI ) );
}

// over the menu, while it's up
void CL_EmotePreview_Draw( void ) {
	const float x = EP_BOX_X, y = EP_BOX_Y, w = EP_BOX_W, h = EP_BOX_H;
	const epEmote_t *e;

	if ( !EP_Shown() )
		return;
	EP_Load();
	e = EP_Find( ep.shown );
	ep.font = re->RegisterFont( "ocr_a" );
	if ( !ep.font )
		ep.font = cls.menuFont;

	EP_Fill( x, y, w, h, epPanel );
	EP_Fill( x, y, w, 1, epBorder );
	EP_Fill( x, y + h - 1, w, 1, epBorder );
	EP_Fill( x, y, 1, h, epBorder );
	EP_Fill( x + w - 1, y, 1, h, epBorder );
	EP_Text( x, y + 4, w, ep.shown, epWhite );

	if ( !e ) {
		EP_Text( x, y + h * 0.5f - 14, w, "Not seen yet:", epDim );
		EP_Text( x, y + h * 0.5f, w, va( "do /%s once", ep.shown ), epDim );
		EP_Text( x, y + h * 0.5f + 14, w, "and it shows here", epDim );
		return;
	}
	EP_Update( e );
	if ( ep.g2 && !ep.error && ep.cycleMs && cls.realtime - ep.animStart > ep.cycleMs )
		EP_Animate( e );
	if ( ep.error || !ep.g2 ) {
		EP_Text( x, y + h * 0.5f, w, ep.error ? ep.error : "Couldn't show it", epDim );
		return;
	}
	EP_Render( x + 2, y + 20, w - 4, h - 38 );
	EP_TextScaled( x, y + h - 16, w, 0.55f, e->torso, epDim );
}

void CL_EmotePreview_Frame( void ) {
	// the menu closed: the model goes, now the last scene with it has been drawn
	if ( !EP_Shown() ) {
		ep.shown[0] = '\0';
		if ( ep.g2 )
			EP_Free();
	}

	// watch: every change for a while after an emote
	if ( ep.watch && ep.watched[0] && cl.snap.valid ) {
		if ( cls.realtime - ep.watchTime > EP_WATCH_MS ) {
			ep.watched[0] = '\0';
		} else if ( cl.snap.ps.torsoAnim != ep.watchTorso || cl.snap.ps.torsoFlip != ep.watchTorsoFlip
			|| cl.snap.ps.legsAnim != ep.watchLegs || cl.snap.ps.legsFlip != ep.watchLegsFlip ) {
			ep.watchTorso = cl.snap.ps.torsoAnim;
			ep.watchLegs = cl.snap.ps.legsAnim;
			ep.watchTorsoFlip = cl.snap.ps.torsoFlip;
			ep.watchLegsFlip = cl.snap.ps.legsFlip;
			Com_Printf( "%s +%ims: torso %s (%i), legs %s (%i)\n", ep.watched, cls.realtime - ep.watchTime,
				EP_AnimName( ep.watchTorso ), ep.watchTorso, EP_AnimName( ep.watchLegs ), ep.watchLegs );
		}
	}

	// an emote just done: its animation, once the snapshot shows it changed
	if ( !ep.learning[0] || !cl.snap.valid )
		return;
	if ( cls.realtime - ep.learnTime > EP_LEARN_MS ) {
		ep.learning[0] = '\0';
		return;
	}
	if ( cl.snap.ps.torsoAnim != ep.torso || cl.snap.ps.torsoFlip != ep.torsoFlip
		|| cl.snap.ps.legsAnim != ep.legs || cl.snap.ps.legsFlip != ep.legsFlip ) {
		const char *torso = EP_AnimName( cl.snap.ps.torsoAnim ), *legs = EP_AnimName( cl.snap.ps.legsAnim );

		if ( torso && legs )
			EP_Learned( ep.learning, torso, legs );
		ep.learning[0] = '\0';
	}
}

static void EP_Emotes_f( void ) {
	EP_Load();
	if ( !Q_stricmp( Cmd_Argv( 1 ), "watch" ) ) {
		ep.watch = (qboolean)!ep.watch;
		ep.watched[0] = '\0';
		Com_Printf( ep.watch ? "Watching: after an emote, each change to your animation is printed for %i seconds\n"
			: "Not watching\n", EP_WATCH_MS / 1000 );
		return;
	}
	if ( !Q_stricmp( Cmd_Argv( 1 ), "forget" ) && Cmd_Argv( 2 )[0] ) {
		epEmote_t *e = EP_Find( Cmd_Argv( 2 ) );

		if ( !e ) {
			Com_Printf( "No emote %s learned\n", Cmd_Argv( 2 ) );
			return;
		}
		memmove( e, e + 1, ( epNumEmotes - ( e - epEmotes ) - 1 ) * sizeof( *e ) );
		epNumEmotes--;
		EP_Save();
		// a built in one shows again till it's learned
		EP_AddBuiltIn();
		ep.g2Key[0] = '\0';
		Com_Printf( "Forgot %s: do it again to learn it%s\n", Cmd_Argv( 2 ), EP_Find( Cmd_Argv( 2 ) ) ? ", the built in one shows till then" : "" );
		return;
	}
	Com_Printf( "Emotes known, shown in Help, Emote Commands (learned here go in " EP_FILE "):\n" );
	for ( int i = 0; i < epNumEmotes; i++ ) {
		const char *from = epEmotes[i].builtIn ? "" : S_COLOR_GREEN " (learned here)";

		if ( Q_stricmp( epEmotes[i].torso, epEmotes[i].legs ) )
			Com_Printf( "  %-16s %s, legs %s%s\n", epEmotes[i].emote, epEmotes[i].torso, epEmotes[i].legs, from );
		else
			Com_Printf( "  %-16s %s%s\n", epEmotes[i].emote, epEmotes[i].torso, from );
	}
	if ( !epNumEmotes )
		Com_Printf( "  none yet: do an emote and it's learned\n" );
	Com_Printf( "emotepreview forget <emote> : learn one again\n"
		"emotepreview watch : print your animation's changes after an emote, to see what it plays\n" );
}

void CL_EmotePreview_Init( void ) {
	memset( &ep, 0, sizeof( ep ) );
	epLoaded = qfalse;
	Cmd_AddCommand( "emotepreview", EP_Emotes_f, "The emotes learned for the Help menu's preview, and their animations" );
}

// cgame is going away, and the renderer's models with it
void CL_EmotePreview_Shutdown( void ) {
	EP_Free();
	ep.shown[0] = '\0';
	ep.learning[0] = '\0';
}
