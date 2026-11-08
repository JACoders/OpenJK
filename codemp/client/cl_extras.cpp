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

// cl_extras.cpp -- people only this client draws, for the cinematic manager's cast
//
// An extra is a player model the server knows nothing of: any model and skin, with a saber or
// not, standing at its spot until the cinematic poses it somewhere else. It goes by an entity
// number past any real one (CL_EXTRA_NUM_BASE + its slot), so the cinematic moves, animates,
// records and looks after it as it does a player or an NPC: cl_npcwalk.cpp hands its poses here.
// It walks and runs as it's moved, plays what a recording or the timeline gives it, and is drawn
// through CL_RpCinematic_Entity, as the cgame's own models are, for animations, tilts and looks.

#include "client.h"
#include "ghoul2/G2.h"
#include "cl_rpcinematic.h"

#define EX_SABER_SINGLE		"models/weapons2/saber_1/saber_1.glm"
#define EX_SABER_STAFF		"models/weapons2/saber_dual_1/saber_dual_1.glm"
#define EX_SINGLE_LEN		40.0f
#define EX_STAFF_LEN		32.0f

static const char *exColors[CL_EXTRA_NUM_COLORS] = { "red", "orange", "yellow", "green", "blue", "purple" };

typedef struct {
	qboolean		defined, shown;
	char			name[MAX_QPATH];		// as the cinematic has it, its + too
	char			model[MAX_QPATH];		// "model" or "model/skin"
	int				saber;					// clExtraSaber_t
	int				color;
	vec3_t			home;
	float			homeYaw;

	// this frame: where the cinematic has it, how fast it goes, what a recording has it play
	vec3_t			pos, ang;
	int				pace;
	qboolean		haveState;
	int				legsAnim, torsoAnim;

	// its model, made again when the model or saber changes
	CGhoul2Info_v	*g2;
	char			key[MAX_QPATH * 2];
	int				cfg;					// its animation.cfg (RC_AnimCfg), -1 none
	int				blades[2][2];			// a saber's model and bolt, each blade
	int				numBlades;
	float			bladeLen;

	// what it plays, legs and torso, and since when
	char			legsName[48], torsoName[48];
	int				legsStart, torsoStart;
} extra_t;

static extra_t exExtras[CL_MAX_EXTRAS];

qboolean CL_Extras_IsNum( int num ) {
	return (qboolean)( num >= CL_EXTRA_NUM_BASE && num < CL_EXTRA_NUM_BASE + CL_MAX_EXTRAS );
}

static extra_t *EX_ByNum( int num ) {
	if ( !CL_Extras_IsNum( num ) || !exExtras[num - CL_EXTRA_NUM_BASE].defined )
		return NULL;
	return &exExtras[num - CL_EXTRA_NUM_BASE];
}

static void EX_FreeModel( extra_t *x ) {
	if ( x->g2 )
		re->G2API_CleanGhoul2Models( &x->g2 );
	x->g2 = NULL;
	x->key[0] = '\0';
	x->legsName[0] = x->torsoName[0] = '\0';
}

void CL_Extras_Clear( void ) {
	for ( int i = 0; i < CL_MAX_EXTRAS; i++ ) {
		EX_FreeModel( &exExtras[i] );
		exExtras[i].defined = qfalse;
	}
}

// the renderer's going: its models with it
void CL_Extras_Shutdown( void ) {
	CL_Extras_Clear();
}

// slot's extra, as the cinematic has it; a slot not defined again is let go by CL_Extras_Keep
void CL_Extras_Define( int slot, const char *name, const char *model, int saber, int color, const vec3_t home, float yaw, qboolean shown ) {
	extra_t *x;

	if ( slot < 0 || slot >= CL_MAX_EXTRAS )
		return;
	x = &exExtras[slot];
	if ( !x->defined ) {
		VectorCopy( home, x->pos );
		VectorSet( x->ang, 0, yaw, 0 );
	}
	x->defined = qtrue;
	x->shown = shown;
	Q_strncpyz( x->name, name, sizeof( x->name ) );
	Q_strncpyz( x->model, model[0] ? model : "kyle", sizeof( x->model ) );
	x->saber = Com_Clampi( 0, CL_EXTRA_SABER_TWO, saber );
	x->color = Com_Clampi( 0, CL_EXTRA_NUM_COLORS - 1, color );
	VectorCopy( home, x->home );
	x->homeYaw = yaw;
}

// the slots defined this frame, kept; the rest let go
void CL_Extras_Keep( const qboolean *keep ) {
	for ( int i = 0; i < CL_MAX_EXTRAS; i++ ) {
		if ( exExtras[i].defined && !keep[i] ) {
			EX_FreeModel( &exExtras[i] );
			exExtras[i].defined = qfalse;
		}
	}
}

// the extra by the name the cinematic gives it; its entity number, -1 none
int CL_Extras_Find( const char *name ) {
	for ( int i = 0; i < CL_MAX_EXTRAS; i++ ) {
		if ( exExtras[i].defined && !Q_stricmp( exExtras[i].name, name ) )
			return CL_EXTRA_NUM_BASE + i;
	}
	return -1;
}

// each frame before the cinematic poses them: back at their spot, standing
void CL_Extras_ResetPoses( void ) {
	for ( int i = 0; i < CL_MAX_EXTRAS; i++ ) {
		extra_t *x = &exExtras[i];

		VectorCopy( x->home, x->pos );
		VectorSet( x->ang, 0, x->homeYaw, 0 );
		x->pace = 0;
		x->haveState = qfalse;
	}
}

void CL_Extras_Pose( int num, const vec3_t pos, const vec3_t angles, int pace, const npcPoseState_t *st ) {
	extra_t *x = EX_ByNum( num );

	if ( !x )
		return;
	VectorCopy( pos, x->pos );
	VectorCopy( angles, x->ang );
	x->pace = pace;
	x->haveState = (qboolean)( st != NULL );
	if ( st ) {
		// started again: from its first frame
		if ( st->legsFlip || x->legsAnim != st->legsAnim )
			x->legsName[0] = '\0';
		if ( st->torsoFlip || x->torsoAnim != st->torsoAnim )
			x->torsoName[0] = '\0';
		x->legsAnim = st->legsAnim;
		x->torsoAnim = st->torsoAnim;
	}
}

void CL_Extras_Show( int num, qboolean shown ) {
	extra_t *x = EX_ByNum( num );

	if ( x )
		x->shown = shown;
}

qboolean CL_Extras_Origin( int num, vec3_t out ) {
	const extra_t *x = EX_ByNum( num );

	if ( !x )
		return qfalse;
	VectorCopy( x->pos, out );
	return qtrue;
}

qboolean CL_Extras_Yaw( int num, float *yaw ) {
	const extra_t *x = EX_ByNum( num );

	if ( !x )
		return qfalse;
	*yaw = x->ang[YAW];
	return qtrue;
}

// the ones drawn, to tell whose a body or a blade is
int CL_Extras_Bodies( int *nums, vec3_t *orgs, int max ) {
	int n = 0;

	for ( int i = 0; i < CL_MAX_EXTRAS && n < max; i++ ) {
		if ( exExtras[i].defined && exExtras[i].shown && exExtras[i].g2 ) {
			nums[n] = CL_EXTRA_NUM_BASE + i;
			VectorCopy( exExtras[i].pos, orgs[n] );
			n++;
		}
	}
	return n;
}

// a saber in a hand: its model after the body's, bolted to it, and its blades' bolts
static void EX_AddSaber( extra_t *x, const char *glm, const char *hand, int blades ) {
	const int model = re->G2API_InitGhoul2Model( &x->g2, glm, 0, 0, 0, 0, 0 );
	int bolt;

	if ( model <= 0 )
		return;
	bolt = re->G2API_AddBolt( *x->g2, 0, hand );
	if ( bolt >= 0 )
		re->G2API_SetBoltInfo( *x->g2, model, bolt );
	for ( int b = 0; b < blades && x->numBlades < 2; b++ ) {
		const int blade = re->G2API_AddBolt( *x->g2, model, va( "*blade%i", b + 1 ) );

		if ( blade >= 0 ) {
			x->blades[x->numBlades][0] = model;
			x->blades[x->numBlades][1] = blade;
			x->numBlades++;
		}
	}
}

// its model, skin and sabers, made when they change
static void EX_Build( extra_t *x ) {
	char key[sizeof( x->key )], name[MAX_QPATH], glm[MAX_QPATH], skinPath[MAX_QPATH], *slash;
	const char *skin = "default", *gla;
	qhandle_t hSkin;

	Com_sprintf( key, sizeof( key ), "%s|%i", x->model, x->saber );
	if ( !Q_stricmp( key, x->key ) )
		return;
	EX_FreeModel( x );
	Q_strncpyz( x->key, key, sizeof( x->key ) );
	x->cfg = -1;
	x->numBlades = 0;

	Q_strncpyz( name, x->model, sizeof( name ) );
	if ( ( slash = strchr( name, '/' ) ) != NULL ) {
		*slash = '\0';
		if ( slash[1] )
			skin = slash + 1;
	}
	Com_sprintf( glm, sizeof( glm ), "models/players/%s/model.glm", name );
	if ( FS_ReadFile( glm, NULL ) <= 0 ) {
		Com_sprintf( glm, sizeof( glm ), "models/players/kyle/model.glm" );
		Q_strncpyz( name, "kyle", sizeof( name ) );
		skin = "default";
	}
	if ( strchr( skin, '|' ) ) {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/|%s", name, skin );
	} else {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_%s.skin", name, skin );
		if ( FS_ReadFile( skinPath, NULL ) <= 0 )
			Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_default.skin", name );
	}
	hSkin = re->RegisterSkin( skinPath );
	if ( re->G2API_InitGhoul2Model( &x->g2, glm, 0, hSkin, 0, 0, 0 ) < 0 || !x->g2 ) {
		EX_FreeModel( x );
		Q_strncpyz( x->key, key, sizeof( x->key ) );	// not again every frame
		return;
	}
	re->G2API_SetSkin( *x->g2, 0, hSkin, hSkin );
	gla = re->G2API_GetGLAName( *x->g2, 0 );
	x->cfg = gla && gla[0] ? RC_AnimCfg( gla ) : -1;
	if ( x->saber == CL_EXTRA_SABER_SINGLE || x->saber == CL_EXTRA_SABER_TWO )
		EX_AddSaber( x, EX_SABER_SINGLE, "*r_hand", 1 );
	if ( x->saber == CL_EXTRA_SABER_TWO )
		EX_AddSaber( x, EX_SABER_SINGLE, "*l_hand", 1 );
	if ( x->saber == CL_EXTRA_SABER_STAFF )
		EX_AddSaber( x, EX_SABER_STAFF, "*r_hand", 2 );
	x->bladeLen = x->saber == CL_EXTRA_SABER_STAFF ? EX_STAFF_LEN : EX_SINGLE_LEN;
}

// an animation on a bone, from when it started: round and round if it's a stand, a walk or a run,
// else held on its last frame
static void EX_Play( extra_t *x, const char *anim, const char *bone, char *playing, int *start ) {
	const rcAnimDef_t *d;
	const qboolean loop = (qboolean)( Q_stristr( anim, "STAND" ) || Q_stristr( anim, "WALK" ) || Q_stristr( anim, "RUN" ) );

	if ( x->cfg < 0 || !( d = RC_FindAnim( x->cfg, anim ) ) || d->num <= 0 || !d->fps )
		return;
	if ( Q_stricmp( playing, anim ) ) {
		Q_strncpyz( playing, anim, 48 );
		*start = cl.serverTime;
	}
	if ( d->fps > 0.0f )
		re->G2API_SetBoneAnim( *x->g2, 0, bone, d->first, d->first + d->num, loop ? BONE_ANIM_OVERRIDE_LOOP : BONE_ANIM_OVERRIDE_FREEZE,
			d->fps / 20.0f, re->G2API_GetTime( *start ), -1, 150 );
	else
		re->G2API_SetBoneAnim( *x->g2, 0, bone, d->first + d->num, d->first, loop ? BONE_ANIM_OVERRIDE_LOOP : BONE_ANIM_OVERRIDE_FREEZE,
			d->fps / 20.0f, re->G2API_GetTime( *start ), -1, 150 );
}

// standing, walking or running as it's moved, or what a recording has it doing
static void EX_Animate( extra_t *x ) {
	const char *legs, *torso;

	if ( x->haveState ) {
		legs = CL_EmotePreview_AnimName( x->legsAnim );
		torso = CL_EmotePreview_AnimName( x->torsoAnim );
	} else {
		static const char *plain[3] = { "BOTH_STAND1", "BOTH_WALK1", "BOTH_RUN1" }, *armed[3] = { "BOTH_STAND2", "BOTH_WALK2", "BOTH_RUN2" };
		const int pace = Com_Clampi( 0, 2, x->pace );

		legs = torso = x->saber ? armed[pace] : plain[pace];
	}
	if ( legs )
		EX_Play( x, legs, "model_root", x->legsName, &x->legsStart );
	if ( torso )
		EX_Play( x, torso, "lower_lumbar", x->torsoName, &x->torsoStart );
}

// a blade from its bolt, as the cgame draws one: the glow and the hot core
static void EX_Blade( const extra_t *x, int b, const vec3_t angles, const vec3_t origin ) {
	mdxaBone_t m;
	vec3_t org, dir, zero = { 0, 0, 0 };
	refEntity_t glow, core, copy;
	const refEntity_t *e;

	if ( !re->G2API_GetBoltMatrix( *x->g2, x->blades[b][0], x->blades[b][1], &m, angles, origin, cl.serverTime, NULL, zero ) )
		return;
	VectorSet( org, m.matrix[0][3], m.matrix[1][3], m.matrix[2][3] );
	VectorSet( dir, -m.matrix[0][1], -m.matrix[1][1], -m.matrix[2][1] );
	VectorNormalize( dir );

	memset( &glow, 0, sizeof( glow ) );
	glow.reType = RT_SABER_GLOW;
	VectorCopy( org, glow.origin );
	VectorCopy( dir, glow.axis[0] );
	glow.saberLength = x->bladeLen;
	glow.radius = 2.6f;
	glow.customShader = re->RegisterShader( va( "gfx/effects/sabers/%s_glow", exColors[x->color] ) );
	glow.shaderRGBA[0] = glow.shaderRGBA[1] = glow.shaderRGBA[2] = glow.shaderRGBA[3] = 255;
	if ( ( e = CL_RpCinematic_Entity( &glow, &copy ) ) != NULL )
		re->AddRefEntityToScene( e );

	memset( &core, 0, sizeof( core ) );
	core.reType = RT_LINE;
	VectorMA( org, x->bladeLen, dir, core.origin );
	VectorMA( org, -1.0f, dir, core.oldorigin );
	core.radius = 0.9f;
	core.customShader = re->RegisterShader( va( "gfx/effects/sabers/%s_line", exColors[x->color] ) );
	core.shaderTexCoord[0] = core.shaderTexCoord[1] = 1.0f;
	core.shaderRGBA[0] = core.shaderRGBA[1] = core.shaderRGBA[2] = core.shaderRGBA[3] = 255;
	if ( ( e = CL_RpCinematic_Entity( &core, &copy ) ) != NULL )
		re->AddRefEntityToScene( e );
}

// in every main view while a cinematic has them
void CL_Extras_AddToScene( const refdef_t *fd ) {
	if ( fd->rdflags & ( RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL | RDF_AUTOMAP ) || cls.state != CA_ACTIVE )
		return;
	for ( int i = 0; i < CL_MAX_EXTRAS; i++ ) {
		extra_t *x = &exExtras[i];
		refEntity_t ent, copy;
		const refEntity_t *e;

		if ( !x->defined || !x->shown )
			continue;
		EX_Build( x );
		if ( !x->g2 )
			continue;
		EX_Animate( x );
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_MODEL;
		ent.ghoul2 = x->g2;
		VectorCopy( x->pos, ent.origin );
		VectorCopy( x->pos, ent.oldorigin );
		VectorCopy( x->pos, ent.lightingOrigin );
		VectorCopy( x->ang, ent.angles );
		AnglesToAxis( x->ang, ent.axis );
		ent.radius = 64.0f;
		ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;
		// the timeline's animations, a look, as the cgame's models have them
		if ( !( e = CL_RpCinematic_Entity( &ent, &copy ) ) )
			continue;
		re->AddRefEntityToScene( e );
		for ( int b = 0; b < x->numBlades; b++ )
			EX_Blade( x, b, x->ang, x->pos );
	}
}
