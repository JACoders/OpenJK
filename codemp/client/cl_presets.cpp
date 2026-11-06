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

// cl_presets.cpp -- characters saved and swapped to in one click (/presets, or PRESETS in the
// in-game menu)
//
// A preset is who you are: your name, model and skin, its colours, and your sabers and their
// colours, saved with the date. Each shows as a card: the name, the character standing with its
// hilts in hand, the skin, the sabers and when it was saved. One clicked becomes you at once.
// Kept in presets.txt. A star makes one a favorite; they're searched (name, model, sabers), shown
// favorites only, and paged through.
//
// /skinselect shows the skins of your model the same way, every model_<skin>.skin of it, worn in a
// click; a model with head_, torso_ and lower_ skins has a card to put them together, a part at
// a time. All models shows every model installed, as it is: worn as it is, or clicked for its skins.

#include "client.h"
#include "ghoul2/G2.h"
#include "cl_widgets.h"

#define PR_FILE				"presets.txt"
#define PR_CMD				"presets"
#define PR_SKIN_CMD			"skinselect"
#define PR_MAX				256
#define PR_MAX_MODELS		1024
#define PR_MAX_SKINS		256
#define PR_MAX_PARTS		128
#define PR_VALUE_LEN		64
#define PR_MESSAGE_MS		5000
#define PR_CONFIRM_MS		3000

#define PR_TEXT_SCALE		0.8f
#define PR_CTRL_H			15.0f
#define PR_COLS				4
#define PR_ROWS				2
#define PR_CARD_W			146.0f
#define PR_CARD_H			186.0f
#define PR_CARD_GAP			6.0f
#define PR_GRID_X			( ( SCREEN_WIDTH - ( PR_COLS * PR_CARD_W + ( PR_COLS - 1 ) * PR_CARD_GAP ) ) * 0.5f )
#define PR_GRID_Y			60.0f
#define PR_VIEW_H			110.0f

// what a preset keeps: the userinfo that makes a character
static const char *prKeys[] = {
	"name", "model", "char_color_red", "char_color_green", "char_color_blue",
	"saber1", "saber2", "color1", "color2"
};
#define PR_NUM_KEYS			( (int)ARRAY_LEN( prKeys ) )
enum { PR_K_NAME, PR_K_MODEL, PR_K_RED, PR_K_GREEN, PR_K_BLUE, PR_K_SABER1, PR_K_SABER2, PR_K_COLOR1, PR_K_COLOR2 };

typedef struct {
	char		values[PR_NUM_KEYS][PR_VALUE_LEN];
	char		date[32];
	qboolean	favorite;
} prPreset_t;

enum { PR_MODE_PRESETS, PR_MODE_SKINS, PR_MODE_MODELS };

// a card's character, built once for its model and sabers
typedef struct {
	char			key[MAX_QPATH * 4];
	CGhoul2Info_v	*g2;
	vec3_t			mins, maxs;
	qboolean		measured;		// from its head and its feet, posed
	float			bottom, top;
	const char		*error;
} prView_t;

static prPreset_t	prPresets[PR_MAX];
static int			prNumPresets;
static prView_t		prViews[PR_MAX];
static prView_t		prSlotViews[8];		// a model's or a skin's card, by its place on the page

static char			prModels[PR_MAX_MODELS][MAX_QPATH];
static int			prNumModels;
static char			prSkins[PR_MAX_SKINS][MAX_QPATH];	// a model's: model_<this>.skin
static int			prNumSkins;
static char			prHeads[PR_MAX_PARTS][MAX_QPATH], prTorsos[PR_MAX_PARTS][MAX_QPATH], prLowers[PR_MAX_PARTS][MAX_QPATH];
static int			prNumHeads, prNumTorsos, prNumLowers;

static struct {
	qboolean	open;
	qhandle_t	font;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	int			page;
	char		message[256];
	int			messageTime;
	int			confirm, confirmTime;	// the card whose Delete was clicked once
	int			mode;
	char		search[48];
	qboolean	typing;
	qboolean	favOnly;
	int			shown[PR_MAX_MODELS];	// what the pages show, filtered: presets, skins (-1 the parts card) or models
	int			numShown;
	char		skinModel[MAX_QPATH];	// the model whose skins are shown
	int			head, torso, lower;		// the parts card's
	qboolean	modelsListed;
} pr;

static vec4_t prWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t prPanel		= { 0.0f, 0.0f, 0.0f, 0.7f };
static vec4_t prPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static vec4_t prCard		= { 0.06f, 0.06f, 0.09f, 0.92f };
static vec4_t prCardHover	= { 0.12f, 0.16f, 0.24f, 0.95f };
static vec4_t prHoverBox	= { 0.25f, 0.25f, 0.25f, 0.9f };
static vec4_t prDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static vec4_t prBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static vec4_t prDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t prAccent		= { 1.0f, 0.8f, 0.3f, 1.0f };
static vec4_t prAccentFill	= { 0.45f, 0.35f, 0.1f, 0.9f };

static void PR_Message( const char *text ) {
	Q_strncpyz( pr.message, text, sizeof( pr.message ) );
	pr.messageTime = cls.realtime;
}

/*
===============================================================================

FILE

===============================================================================
*/

static void PR_FreeView( int i ) {
	if ( prViews[i].g2 )
		re->G2API_CleanGhoul2Models( &prViews[i].g2 );
	memset( &prViews[i], 0, sizeof( prViews[i] ) );
}

static void PR_FreeViews( void ) {
	for ( int i = 0; i < PR_MAX; i++ )
		PR_FreeView( i );
	for ( int i = 0; i < (int)ARRAY_LEN( prSlotViews ); i++ ) {
		if ( prSlotViews[i].g2 )
			re->G2API_CleanGhoul2Models( &prSlotViews[i].g2 );
		memset( &prSlotViews[i], 0, sizeof( prSlotViews[i] ) );
	}
}

static void PR_Save( void ) {
	const fileHandle_t f = FS_FOpenFileWrite( PR_FILE );

	if ( !f ) {
		PR_Message( S_COLOR_YELLOW "Couldn't write " PR_FILE );
		return;
	}
	{
		const char *head = "// Presets: a character a block\n";

		FS_Write( head, strlen( head ), f );
	}
	for ( int i = 0; i < prNumPresets; i++ ) {
		const char *head = va( "preset \"%s\"\n{\n", prPresets[i].date );

		FS_Write( head, strlen( head ), f );
		for ( int k = 0; k < PR_NUM_KEYS; k++ ) {
			const char *line = va( "\t%s \"%s\"\n", prKeys[k], prPresets[i].values[k] );

			FS_Write( line, strlen( line ), f );
		}
		if ( prPresets[i].favorite )
			FS_Write( "\tfavorite \"1\"\n", (int)strlen( "\tfavorite \"1\"\n" ), f );
		FS_Write( "}\n", 2, f );
	}
	FS_FCloseFile( f );
}

static void PR_Load( void ) {
	char *buf;
	const char *p;

	prNumPresets = 0;
	if ( FS_ReadFile( PR_FILE, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( PR_FILE );
	while ( prNumPresets < PR_MAX ) {
		const char *token = COM_ParseExt( &p, qtrue );
		prPreset_t *ps;

		if ( !token[0] )
			break;
		if ( Q_stricmp( token, "preset" ) )
			continue;
		ps = &prPresets[prNumPresets];
		memset( ps, 0, sizeof( *ps ) );
		Q_strncpyz( ps->date, COM_ParseExt( &p, qtrue ), sizeof( ps->date ) );
		token = COM_ParseExt( &p, qtrue );
		if ( strcmp( token, "{" ) )
			break;
		while ( 1 ) {
			char key[64];

			token = COM_ParseExt( &p, qtrue );
			if ( !token[0] || !strcmp( token, "}" ) )
				break;
			Q_strncpyz( key, token, sizeof( key ) );
			token = COM_ParseExt( &p, qfalse );
			if ( !Q_stricmp( key, "favorite" ) )
				ps->favorite = (qboolean)( atoi( token ) != 0 );
			for ( int k = 0; k < PR_NUM_KEYS; k++ ) {
				if ( !Q_stricmp( key, prKeys[k] ) )
					Q_strncpyz( ps->values[k], token, PR_VALUE_LEN );
			}
		}
		if ( ps->values[PR_K_MODEL][0] )
			prNumPresets++;
	}
	FS_FreeFile( buf );
}

/*
===============================================================================

SAVING AND SWAPPING

===============================================================================
*/

// who you are now
static void PR_FromCurrent( prPreset_t *ps ) {
	qtime_t t;

	memset( ps, 0, sizeof( *ps ) );
	for ( int k = 0; k < PR_NUM_KEYS; k++ )
		Q_strncpyz( ps->values[k], Cvar_VariableString( prKeys[k] ), PR_VALUE_LEN );
	Com_RealTime( &t );
	Com_sprintf( ps->date, sizeof( ps->date ), "%04i-%02i-%02i %02i:%02i", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min );
}

static void PR_SaveCurrent( void ) {
	if ( prNumPresets >= PR_MAX ) {
		PR_Message( S_COLOR_YELLOW "There are as many presets as there can be: delete one first" );
		return;
	}
	PR_FromCurrent( &prPresets[prNumPresets++] );
	PR_Save();
	pr.page = ( prNumPresets - 1 ) / ( PR_COLS * PR_ROWS );
	PR_Message( va( S_COLOR_GREEN "Saved %s" S_COLOR_GREEN " as a preset", prPresets[prNumPresets - 1].values[PR_K_NAME] ) );
}

// a preset becomes you: what differs is set, and goes to the server as one change
static void PR_Apply( int i ) {
	const prPreset_t *ps = &prPresets[i];

	for ( int k = 0; k < PR_NUM_KEYS; k++ ) {
		if ( strcmp( Cvar_VariableString( prKeys[k] ), ps->values[k] ) )
			Cvar_Set( prKeys[k], ps->values[k] );
	}
	Com_Printf( "Presets: you're %s" S_COLOR_WHITE " now (%s)\n", ps->values[PR_K_NAME], ps->values[PR_K_MODEL] );
}

static void PR_Delete( int i ) {
	PR_FreeViews();
	memmove( &prPresets[i], &prPresets[i + 1], ( prNumPresets - i - 1 ) * sizeof( prPresets[0] ) );
	prNumPresets--;
	PR_Save();
}

/*
===============================================================================

THE CHARACTER ON A CARD

===============================================================================
*/

// a saber's hilt model, from the .sab file that has it
static qboolean PR_SaberModel( const char *saber, char *model, int size ) {
	char **list;
	int n;
	qboolean found = qfalse;

	model[0] = '\0';
	if ( !saber[0] || !Q_stricmp( saber, "none" ) )
		return qfalse;
	list = FS_ListFiles( "ext_data/sabers", ".sab", &n );
	for ( int i = 0; i < n && !found; i++ ) {
		char *buf;
		const char *p;

		if ( FS_ReadFile( va( "ext_data/sabers/%s", list[i] ), (void **)&buf ) <= 0 || !buf )
			continue;
		p = buf;
		COM_BeginParseSession( list[i] );
		while ( !found ) {
			const char *token = COM_ParseExt( &p, qtrue );

			if ( !token[0] )
				break;
			if ( Q_stricmp( token, saber ) ) {
				if ( !strcmp( token, "{" ) )
					SkipBracedSection( &p, 1 );
				continue;
			}
			token = COM_ParseExt( &p, qtrue );
			if ( strcmp( token, "{" ) )
				continue;
			// this one's block: its saberModel
			while ( 1 ) {
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] || !strcmp( token, "}" ) )
					break;
				if ( !strcmp( token, "{" ) ) {
					SkipBracedSection( &p, 1 );
					continue;
				}
				if ( !Q_stricmp( token, "saberModel" ) ) {
					Q_strncpyz( model, COM_ParseExt( &p, qfalse ), size );
					break;
				}
			}
			found = qtrue;
		}
		FS_FreeFile( buf );
	}
	FS_FreeFileList( list );
	return (qboolean)( model[0] != '\0' );
}

// an animation from the animation.cfg beside the model's skeleton, held still on the whole body
static void PR_Pose( CGhoul2Info_v &g2, const char *anim ) {
	char path[MAX_QPATH], *buf, *slash;
	const char *gla = re->G2API_GetGLAName( g2, 0 ), *p;

	if ( !gla || !gla[0] )
		return;
	Q_strncpyz( path, gla, sizeof( path ) );
	if ( !( slash = strrchr( path, '/' ) ) )
		return;
	slash[1] = '\0';
	Q_strcat( path, sizeof( path ), "animation.cfg" );
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return;
	p = buf;
	COM_BeginParseSession( path );
	while ( 1 ) {
		const char *token = COM_ParseExt( &p, qtrue );

		if ( !token[0] )
			break;
		if ( Q_stricmp( token, anim ) ) {
			SkipRestOfLine( &p );
			continue;
		}
		{
			const int first = atoi( COM_ParseExt( &p, qfalse ) );

			re->G2API_SetBoneAnim( g2, 0, "model_root", first, first + 1, BONE_ANIM_OVERRIDE_FREEZE, 0.001f,
				re->G2API_GetTime( cl.serverTime ), (float)first, 0 );
		}
		break;
	}
	FS_FreeFile( buf );
}

// a card's model, (re)built when its character changed: a model and skin as the model cvar has
// them ("name/skin", or "name/head|torso|lower"), with its sabers' hilts in its hands
static void PR_BuildView( prView_t *v, const char *model, const char *saber1, const char *saber2 ) {
	char key[sizeof( v->key )], name[MAX_QPATH], glm[MAX_QPATH], skinPath[MAX_QPATH], *slash;
	const char *skin = "default";
	qhandle_t hSkin;
	int bolts[2];

	Com_sprintf( key, sizeof( key ), "%s|%s|%s", model, saber1, saber2 );
	if ( !Q_stricmp( key, v->key ) )
		return;
	if ( v->g2 )
		re->G2API_CleanGhoul2Models( &v->g2 );
	memset( v, 0, sizeof( *v ) );
	Q_strncpyz( v->key, key, sizeof( v->key ) );

	Q_strncpyz( name, model, sizeof( name ) );
	slash = strchr( name, '/' );
	if ( slash ) {
		*slash = '\0';
		if ( slash[1] )
			skin = slash + 1;
	}
	Com_sprintf( glm, sizeof( glm ), "models/players/%s/model.glm", name );
	if ( CL_GlmBounds( glm, v->mins, v->maxs ) ) {
		v->error = "No such model here";
		return;
	}
	if ( strchr( skin, '|' ) ) {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/|%s", name, skin );
	} else {
		Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_%s.skin", name, skin );
		if ( FS_ReadFile( skinPath, NULL ) <= 0 )
			Com_sprintf( skinPath, sizeof( skinPath ), "models/players/%s/model_default.skin", name );
	}
	hSkin = re->RegisterSkin( skinPath );
	if ( re->G2API_InitGhoul2Model( &v->g2, glm, 0, hSkin, 0, 0, 0 ) < 0 || !v->g2 ) {
		v->g2 = NULL;
		v->error = "Couldn't load the model";
		return;
	}
	re->G2API_SetSkin( *v->g2, 0, hSkin, hSkin );

	// the hilts in its hands, as the game puts them: model 1 on the right hand's bolt, 2 on the left's
	bolts[0] = re->G2API_AddBolt( *v->g2, 0, "*r_hand" );
	bolts[1] = re->G2API_AddBolt( *v->g2, 0, "*l_hand" );
	for ( int s = 0; s < 2; s++ ) {
		char hilt[MAX_QPATH];
		int index;

		if ( bolts[s] < 0 || !PR_SaberModel( s ? saber2 : saber1, hilt, sizeof( hilt ) ) )
			continue;
		index = re->G2API_InitGhoul2Model( &v->g2, hilt, s + 1, 0, 0, 0, 0 );
		if ( index > 0 )
			re->G2API_SetBoltInfo( *v->g2, index, bolts[s] );
	}
	PR_Pose( *v->g2, saber1[0] && Q_stricmp( saber1, "none" ) ? "BOTH_STAND2" : "BOTH_STAND1" );
}

// how tall it stands: its head's top to its feet, as posed; the model's bounds when it hasn't the tags
static void PR_Measure( prView_t *v ) {
	static const char *tags[] = { "*head_top", "*r_leg_foot", "*l_leg_foot" };
	vec3_t zero = { 0, 0, 0 };
	float z[3];
	int found = 0;

	v->measured = qtrue;
	v->bottom = v->mins[2];
	v->top = v->maxs[2];
	for ( int t = 0; t < 3; t++ ) {
		const int bolt = re->G2API_AddBolt( *v->g2, 0, tags[t] );
		mdxaBone_t m;

		if ( bolt < 0 || !re->G2API_GetBoltMatrix( *v->g2, 0, bolt, &m, zero, zero, cl.serverTime, NULL, zero ) )
			continue;
		z[t] = m.matrix[2][3];
		found |= 1 << t;
	}
	if ( ( found & 1 ) && ( found & 6 ) ) {
		const float feet = ( found & 2 ) && ( found & 4 ) ? Q_min( z[1], z[2] ) : ( found & 2 ) ? z[1] : z[2];

		v->bottom = feet - 3.0f;
		v->top = z[0] + 3.0f;
	}
}

// the character drawn in the card, tinted as given (a skin that takes a tint)
static void PR_RenderView( prView_t *v, const char *red, const char *green, const char *blue, float x, float y, float w, float h, float yaw ) {
	const float xScale = cls.glconfig.vidWidth / (float)SCREEN_WIDTH, yScale = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;
	refdef_t refdef;
	refEntity_t ent;
	vec3_t center, forward, angles;
	float halfH, fovX, fovY, dist;

	if ( !v->g2 )
		return;
	if ( !v->measured )
		PR_Measure( v );
	memset( &ent, 0, sizeof( ent ) );
	halfH = ( v->top - v->bottom ) * 0.5f;
	VectorSet( center, 0, 0, ( v->top + v->bottom ) * 0.5f );
	VectorSet( ent.angles, 0, yaw, 0 );

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.x = (int)( x * xScale );
	refdef.y = (int)( y * yScale );
	refdef.width = (int)( w * xScale );
	refdef.height = (int)( h * yScale );
	fovY = 30.0f;
	fovX = RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovY * 0.5f ) ) * refdef.width / (float)refdef.height ) );
	refdef.fov_x = fovX;
	refdef.fov_y = fovY;
	dist = Q_max( halfH / tanf( DEG2RAD( fovY * 0.5f ) ), halfH * 0.6f / tanf( DEG2RAD( fovX * 0.5f ) ) ) * 1.08f;
	VectorSet( angles, 0.0f, 180.0f, 0 );
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( center, -dist, forward, refdef.vieworg );
	AnglesToAxis( angles, refdef.viewaxis );
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.time = cl.serverTime;

	ent.reType = RT_MODEL;
	ent.ghoul2 = v->g2;
	ent.renderfx = RF_NOSHADOW;
	AnglesToAxis( ent.angles, ent.axis );
	ent.shaderRGBA[0] = (byte)Com_Clampi( 0, 255, red[0] ? atoi( red ) : 255 );
	ent.shaderRGBA[1] = (byte)Com_Clampi( 0, 255, green[0] ? atoi( green ) : 255 );
	ent.shaderRGBA[2] = (byte)Com_Clampi( 0, 255, blue[0] ? atoi( blue ) : 255 );
	ent.shaderRGBA[3] = 255;
	ent.radius = halfH * 2.0f;

	re->ClearScene();
	re->AddRefEntityToScene( &ent );
	re->RenderScene( &refdef );
}

/*
===============================================================================

SKINS AND MODELS

===============================================================================
*/

static int QDECL PR_CompareNames( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, (const char *)b );
}

// the models installed: each folder of models/players with a model.glm
static void PR_ListModels( void ) {
	static char list[65536];
	const int n = FS_GetFileList( "models/players", "/", list, sizeof( list ) );
	char *p = list;

	prNumModels = 0;
	for ( int i = 0; i < n && prNumModels < PR_MAX_MODELS; i++, p += strlen( p ) + 1 ) {
		char name[MAX_QPATH];

		Q_strncpyz( name, p, sizeof( name ) );
		if ( name[0] && name[strlen( name ) - 1] == '/' )
			name[strlen( name ) - 1] = '\0';
		if ( !name[0] || name[0] == '.' || FS_ReadFile( va( "models/players/%s/model.glm", name ), NULL ) <= 0 )
			continue;
		// a model in several pk3s is listed once
		{
			qboolean seen = qfalse;

			for ( int k = 0; k < prNumModels && !seen; k++ )
				seen = (qboolean)!Q_stricmp( prModels[k], name );
			if ( seen )
				continue;
		}
		Q_strncpyz( prModels[prNumModels++], name, sizeof( prModels[0] ) );
	}
	qsort( prModels, prNumModels, sizeof( prModels[0] ), PR_CompareNames );
	pr.modelsListed = qtrue;
}

// a model's skins: model_<skin>.skin each a skin, head_, torso_ and lower_ the parts a character
// is put together from
static void PR_ListSkins( const char *model ) {
	char **list;
	int n;

	Q_strncpyz( pr.skinModel, model, sizeof( pr.skinModel ) );
	prNumSkins = prNumHeads = prNumTorsos = prNumLowers = 0;
	list = FS_ListFiles( va( "models/players/%s", model ), ".skin", &n );
	for ( int i = 0; i < n; i++ ) {
		char name[MAX_QPATH];

		COM_StripExtension( list[i], name, sizeof( name ) );
		if ( !Q_stricmpn( name, "model_", 6 ) && name[6] && prNumSkins < PR_MAX_SKINS )
			Q_strncpyz( prSkins[prNumSkins++], name + 6, sizeof( prSkins[0] ) );
		else if ( !Q_stricmpn( name, "head_", 5 ) && prNumHeads < PR_MAX_PARTS )
			Q_strncpyz( prHeads[prNumHeads++], name, sizeof( prHeads[0] ) );
		else if ( !Q_stricmpn( name, "torso_", 6 ) && prNumTorsos < PR_MAX_PARTS )
			Q_strncpyz( prTorsos[prNumTorsos++], name, sizeof( prTorsos[0] ) );
		else if ( !Q_stricmpn( name, "lower_", 6 ) && prNumLowers < PR_MAX_PARTS )
			Q_strncpyz( prLowers[prNumLowers++], name, sizeof( prLowers[0] ) );
	}
	FS_FreeFileList( list );
	qsort( prSkins, prNumSkins, sizeof( prSkins[0] ), PR_CompareNames );
	qsort( prHeads, prNumHeads, sizeof( prHeads[0] ), PR_CompareNames );
	qsort( prTorsos, prNumTorsos, sizeof( prTorsos[0] ), PR_CompareNames );
	qsort( prLowers, prNumLowers, sizeof( prLowers[0] ), PR_CompareNames );
	// default first
	for ( int i = 1; i < prNumSkins; i++ ) {
		if ( !Q_stricmp( prSkins[i], "default" ) ) {
			char keep[MAX_QPATH];

			Q_strncpyz( keep, prSkins[i], sizeof( keep ) );
			memmove( prSkins[1], prSkins[0], i * sizeof( prSkins[0] ) );
			Q_strncpyz( prSkins[0], keep, sizeof( prSkins[0] ) );
			break;
		}
	}
	// the parts as worn now, when it's this model in parts; else the first of each
	pr.head = pr.torso = pr.lower = 0;
	{
		char now[MAX_QPATH], *slash;

		Q_strncpyz( now, Cvar_VariableString( "model" ), sizeof( now ) );
		slash = strchr( now, '/' );
		if ( slash && strchr( slash, '|' ) ) {
			char *parts[3] = { slash + 1, NULL, NULL };

			*slash = '\0';
			if ( !Q_stricmp( now, model ) ) {
				parts[1] = strchr( parts[0], '|' );
				if ( parts[1] ) {
					*parts[1]++ = '\0';
					parts[2] = strchr( parts[1], '|' );
					if ( parts[2] )
						*parts[2]++ = '\0';
				}
				for ( int i = 0; i < prNumHeads; i++ )
					if ( !Q_stricmp( prHeads[i], parts[0] ) ) pr.head = i;
				for ( int i = 0; parts[1] && i < prNumTorsos; i++ )
					if ( !Q_stricmp( prTorsos[i], parts[1] ) ) pr.torso = i;
				for ( int i = 0; parts[2] && i < prNumLowers; i++ )
					if ( !Q_stricmp( prLowers[i], parts[2] ) ) pr.lower = i;
			}
		}
	}
}

// the model's parts to put together: a head, a torso and legs
static qboolean PR_HasParts( void ) {
	return (qboolean)( prNumHeads && prNumTorsos && prNumLowers );
}

static const char *PR_PartsValue( void ) {
	return va( "%s/%s|%s|%s", pr.skinModel, prHeads[pr.head], prTorsos[pr.torso], prLowers[pr.lower] );
}

// the model you are, without its skin
static const char *PR_CurrentModel( void ) {
	static char name[MAX_QPATH];
	char *slash;

	Q_strncpyz( name, Cvar_VariableString( "model" ), sizeof( name ) );
	slash = strchr( name, '/' );
	if ( slash )
		*slash = '\0';
	return name[0] ? name : "kyle";
}

static void PR_ShowSkins( const char *model ) {
	PR_ListSkins( model );
	pr.mode = PR_MODE_SKINS;
	pr.page = 0;
	pr.search[0] = '\0';
	pr.typing = qfalse;
}

static void PR_ShowModels( void ) {
	if ( !pr.modelsListed )
		PR_ListModels();
	pr.mode = PR_MODE_MODELS;
	pr.page = 0;
	pr.search[0] = '\0';
	pr.typing = qfalse;
	// on the page of the model you are
	for ( int i = 0; i < prNumModels; i++ ) {
		if ( !Q_stricmp( prModels[i], PR_CurrentModel() ) ) {
			pr.page = i / ( PR_COLS * PR_ROWS );
			break;
		}
	}
}

// the model becomes yours
static void PR_SetModel( const char *value ) {
	Cvar_Set( "model", value );
	PR_Message( va( S_COLOR_GREEN "You're %s now", value ) );
	Com_Printf( "Skin select: you're %s now\n", value );
}

/*
===============================================================================

OPENING AND INPUT

===============================================================================
*/

static void PR_Close( void ) {
	if ( !pr.open )
		return;
	pr.open = qfalse;
	pr.typing = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_PRESETS );
}

void CL_Presets_Close( void ) {
	PR_Close();
}

static qboolean PR_Open( int mode ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "%s: join a server first\n", mode == PR_MODE_PRESETS ? "Presets" : "Skin select" );
		return qfalse;
	}
	if ( !pr.open ) {
		CL_ModelManager_Close();
		CL_NpcManager_Close();
		CL_ShaderManager_Close();
		CL_EffectManager_Close();
		CL_Storyteller_Close();
		CL_RpCinematic_EditorClose();
		CL_MapEditor_Close();
		CL_SoundManager_Close();
		CL_AnimManager_Close();

		pr.font = re->RegisterFont( "arialnb" );
		if ( !pr.font )
			pr.font = cls.menuFont;
		pr.cursorX = SCREEN_WIDTH * 0.5f;
		pr.cursorY = SCREEN_HEIGHT * 0.5f;
		pr.click = qfalse;
		pr.wheel = 0;
		pr.confirm = -1;
		pr.open = qtrue;
		Key_SetCatcher( Key_GetCatcher() | KEYCATCH_PRESETS );
	}
	PR_Load();
	pr.typing = qfalse;
	if ( mode == PR_MODE_PRESETS ) {
		pr.mode = PR_MODE_PRESETS;
		pr.search[0] = '\0';
		pr.page = Com_Clampi( 0, Q_max( 0, ( prNumPresets - 1 ) / ( PR_COLS * PR_ROWS ) ), pr.page );
	} else {
		pr.mode = mode;
	}
	return qtrue;
}

static void PR_Cmd_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		if ( pr.open && pr.mode == PR_MODE_PRESETS )
			PR_Close();
		else
			PR_Open( PR_MODE_PRESETS );
	} else if ( !Q_stricmp( arg, "close" ) ) {
		PR_Close();
	} else if ( !Q_stricmp( arg, "save" ) ) {
		PR_Load();
		PR_SaveCurrent();
		Com_Printf( "%s\n", pr.message );
	} else if ( !Q_stricmp( arg, "list" ) ) {
		PR_Load();
		for ( int i = 0; i < prNumPresets; i++ )
			Com_Printf( "  %i  %s%s" S_COLOR_WHITE "  %s  %s\n", i + 1, prPresets[i].favorite ? S_COLOR_YELLOW "* " S_COLOR_WHITE : "", prPresets[i].values[PR_K_NAME],
				prPresets[i].values[PR_K_MODEL], prPresets[i].date );
	} else if ( atoi( arg ) > 0 ) {
		PR_Load();
		if ( atoi( arg ) <= prNumPresets )
			PR_Apply( atoi( arg ) - 1 );
		else
			Com_Printf( "Presets: there's no preset %s\n", arg );
	} else {
		Com_Printf( "usage: " PR_CMD "          open or close them (the in-game menu's PRESETS)\n" );
		Com_Printf( "       " PR_CMD " save     save who you are now as a preset\n" );
		Com_Printf( "       " PR_CMD " list     the presets, numbered (* a favorite)\n" );
		Com_Printf( "       " PR_CMD " <number> become that one\n" );
	}
}

// /skinselect [model]: the model's skins, yours when none is named
static void PR_SkinSelect_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] && pr.open && pr.mode != PR_MODE_PRESETS ) {
		PR_Close();
		return;
	}
	if ( !Q_stricmp( arg, "models" ) ) {
		if ( PR_Open( PR_MODE_SKINS ) )
			PR_ShowModels();
		return;
	}
	if ( !PR_Open( PR_MODE_SKINS ) )
		return;
	PR_ShowSkins( arg[0] ? arg : PR_CurrentModel() );
	if ( arg[0] && FS_ReadFile( va( "models/players/%s/model.glm", arg ), NULL ) <= 0 )
		PR_Message( va( S_COLOR_YELLOW "There's no model %s here", arg ) );
}

void CL_Presets_Init( void ) {
	memset( &pr, 0, sizeof( pr ) );
	pr.confirm = -1;
	Cmd_AddCommand( PR_CMD, PR_Cmd_f, "Characters saved, and swapped to in one click" );
	Cmd_AddCommand( PR_SKIN_CMD, PR_SkinSelect_f, "Every skin of your model (or the one named), and every model, to wear in one click" );
}

// cgame is going away: the card models with it; the next server may have other models
void CL_Presets_Shutdown( void ) {
	PR_Close();
	PR_FreeViews();
	pr.modelsListed = qfalse;
}

static qboolean PR_IsToggleKey( int key ) {
	const char *binding = Key_GetBinding( key );

	return (qboolean)( VALIDSTRING( binding ) && ( !Q_stricmp( binding, PR_CMD ) || !Q_stricmp( binding, PR_SKIN_CMD ) ) );
}

static int PR_Pages( void ) {
	return Q_max( 1, ( pr.numShown + PR_COLS * PR_ROWS - 1 ) / ( PR_COLS * PR_ROWS ) );
}

void CL_Presets_KeyEvent( int key, qboolean down ) {
	if ( !pr.open || !down )
		return;
	if ( key == A_MOUSE1 ) {
		pr.click = qtrue;
		pr.clickX = pr.cursorX;
		pr.clickY = pr.cursorY;
		return;
	}
	// typed in the search
	if ( pr.typing ) {
		if ( key == A_ENTER || key == A_KP_ENTER ) {
			pr.typing = qfalse;
			return;
		}
		if ( WG_EditKey( pr.search, sizeof( pr.search ), key, NULL ) & WG_EDIT_CHANGED )
			pr.page = 0;
		if ( key != A_MWHEELUP && key != A_MWHEELDOWN && key != A_PAGE_UP && key != A_PAGE_DOWN )
			return;
	}
	if ( PR_IsToggleKey( key ) ) {
		PR_Close();
		return;
	}
	switch ( key ) {
	case A_MWHEELUP:
	case A_CURSOR_LEFT:
	case A_PAGE_UP:
		pr.page = Q_max( 0, pr.page - 1 );
		break;
	case A_MWHEELDOWN:
	case A_CURSOR_RIGHT:
	case A_PAGE_DOWN:
		pr.page = Q_min( PR_Pages() - 1, pr.page + 1 );
		break;
	case A_HOME:
		pr.page = 0;
		break;
	case A_END:
		pr.page = PR_Pages() - 1;
		break;
	default:
		break;
	}
}

void CL_Presets_CharEvent( int ch ) {
	if ( pr.open && pr.typing && WG_EditChar( pr.search, sizeof( pr.search ), ch, NULL ) )
		pr.page = 0;
}

void CL_Presets_MouseEvent( int dx, int dy ) {
	pr.cursorX = Com_Clamp( 0, SCREEN_WIDTH, pr.cursorX + dx );
	pr.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, pr.cursorY + dy );
}

// out of the search first, then a model's skins back to the models, then closed
void CL_Presets_Escape( void ) {
	if ( pr.typing ) {
		pr.typing = qfalse;
		return;
	}
	PR_Close();
}

/*
===============================================================================

DRAWING

===============================================================================
*/

static void PR_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void PR_Box( float x, float y, float w, float h, const float *fill ) {
	PR_Fill( x, y, w, h, fill );
	PR_Fill( x, y, w, 1, prBorder );
	PR_Fill( x, y + h - 1, w, 1, prBorder );
	PR_Fill( x, y, 1, h, prBorder );
	PR_Fill( x + w - 1, y, 1, h, prBorder );
}

static float PR_TextWidth( const char *text, float scale ) {
	return (float)re->Font_StrLenPixels( text, pr.font, scale );
}

// cut to fit, centred in w when centred
static void PR_Text( float x, float y, float w, const char *text, const float *color, float scale, qboolean centred ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && PR_TextWidth( buf, scale ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	if ( centred )
		x += ( w - PR_TextWidth( buf, scale ) ) * 0.5f;
	re->Font_DrawString( (int)x, (int)y, buf, color, pr.font | STYLE_DROPSHADOW, -1, scale );
}

// text in its colours, RPMod's ^a-^z and ^A-^Z too, as the console shows them: a run a colour,
// as the renderer only knows ^0-^9. Cut to fit, centred in w when centred
static void PR_ColorText( float x, float y, float w, const char *text, float scale, qboolean centred ) {
	char plain[MAX_STRING_CHARS], run[MAX_STRING_CHARS];
	vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
	int len = 0, cut, n = 0;

	// the letters alone, to measure
	for ( const char *p = text; *p && len < (int)sizeof( plain ) - 1; p++ ) {
		if ( Con_TypedColor( p ) ) {
			p++;
			continue;
		}
		plain[len++] = *p;
	}
	plain[len] = '\0';
	cut = len;
	while ( cut > 0 && PR_TextWidth( plain, scale ) > w ) {
		plain[--cut] = '\0';
	}
	if ( centred )
		x += ( w - PR_TextWidth( plain, scale ) ) * 0.5f;

	for ( const char *p = text; *p && n < cut; p++ ) {
		const float *c = Con_TypedColor( p );
		int r = 0;

		if ( c ) {
			VectorCopy( c, color );
			p++;
			continue;
		}
		// this colour's letters
		while ( p[r] && !Con_TypedColor( p + r ) && n + r < cut ) {
			run[r] = p[r];
			r++;
		}
		run[r] = '\0';
		re->Font_DrawString( (int)x, (int)y, run, color, pr.font | STYLE_DROPSHADOW, -1, scale );
		x += PR_TextWidth( run, scale );
		n += r;
		p += r - 1;
	}
}

static qboolean PR_InRect( float x, float y, float w, float h ) {
	return (qboolean)( pr.cursorX >= x && pr.cursorX < x + w && pr.cursorY >= y && pr.cursorY < y + h );
}

static qboolean PR_Clicked( float x, float y, float w, float h ) {
	if ( !pr.click || pr.clickX < x || pr.clickX >= x + w || pr.clickY < y || pr.clickY >= y + h )
		return qfalse;
	pr.click = qfalse;
	return qtrue;
}

static qboolean PR_Button( float x, float y, float w, const char *label, const float *fill ) {
	PR_Box( x, y, w, PR_CTRL_H, PR_InRect( x, y, w, PR_CTRL_H ) ? prHoverBox : fill );
	PR_Text( x, y + 2, w, label, prWhite, PR_TEXT_SCALE, qtrue );
	return PR_Clicked( x, y, w, PR_CTRL_H );
}

// a saber's name as it reads: its file name, the colour after it
static const char *PR_SaberText( const prPreset_t *ps ) {
	static const char *colors[] = { "red", "orange", "yellow", "green", "blue", "purple" };
	const char *s1 = ps->values[PR_K_SABER1], *s2 = ps->values[PR_K_SABER2];
	const int c1 = atoi( ps->values[PR_K_COLOR1] ), c2 = atoi( ps->values[PR_K_COLOR2] );
	const char *col1 = c1 >= 0 && c1 < (int)ARRAY_LEN( colors ) ? colors[c1] : ps->values[PR_K_COLOR1];
	const char *col2 = c2 >= 0 && c2 < (int)ARRAY_LEN( colors ) ? colors[c2] : ps->values[PR_K_COLOR2];

	if ( !s1[0] || !Q_stricmp( s1, "none" ) )
		return "no saber";
	if ( !s2[0] || !Q_stricmp( s2, "none" ) )
		return va( "%s (%s)", s1, col1 );
	return va( "%s (%s), %s (%s)", s1, col1, s2, col2 );
}

// a preset matches what's searched: its name (colours left out), its model and skin, its sabers
static qboolean PR_Matches( const prPreset_t *ps ) {
	char name[PR_VALUE_LEN];

	if ( pr.favOnly && !ps->favorite )
		return qfalse;
	if ( !pr.search[0] )
		return qtrue;
	Q_strncpyz( name, ps->values[PR_K_NAME], sizeof( name ) );
	Q_StripColor( name );
	return (qboolean)( Q_stristr( name, pr.search ) || Q_stristr( ps->values[PR_K_MODEL], pr.search )
		|| Q_stristr( ps->values[PR_K_SABER1], pr.search ) || Q_stristr( ps->values[PR_K_SABER2], pr.search ) );
}

static void PR_DrawCard( int i, float x, float y ) {
	const prPreset_t *ps = &prPresets[i];
	const char *slash = strchr( ps->values[PR_K_MODEL], '/' );
	const qboolean hover = PR_InRect( x, y, PR_CARD_W, PR_CARD_H );
	const float bottom = y + PR_CARD_H;
	char model[MAX_QPATH];

	PR_Box( x, y, PR_CARD_W, PR_CARD_H, hover ? prCardHover : prCard );
	// a favorite's star, clicked on and off
	if ( PR_Clicked( x + PR_CARD_W - 18, y + 2, 16, 16 ) ) {
		prPresets[i].favorite = (qboolean)!prPresets[i].favorite;
		PR_Save();
		PR_Message( prPresets[i].favorite ? va( "%s" S_COLOR_WHITE " is a favorite", ps->values[PR_K_NAME] ) : va( "%s" S_COLOR_WHITE " isn't a favorite now", ps->values[PR_K_NAME] ) );
	}
	PR_Text( x + PR_CARD_W - 16, y + 2, 14, "*", ps->favorite ? prAccent : PR_InRect( x + PR_CARD_W - 18, y + 2, 16, 16 ) ? prWhite : prBorder, 1.1f, qtrue );
	// the name over the character, in its colours
	PR_ColorText( x + 16, y + 4, PR_CARD_W - 32, ps->values[PR_K_NAME], 0.9f, qtrue );

	PR_BuildView( &prViews[i], ps->values[PR_K_MODEL], ps->values[PR_K_SABER1], ps->values[PR_K_SABER2] );
	if ( prViews[i].g2 )
		PR_RenderView( &prViews[i], ps->values[PR_K_RED], ps->values[PR_K_GREEN], ps->values[PR_K_BLUE], x + 2, y + 20, PR_CARD_W - 4, PR_VIEW_H,
			hover ? sinf( cls.realtime * 0.002f ) * 35.0f : 20.0f );
	else
		PR_Text( x + 4, y + 20 + PR_VIEW_H * 0.5f - 6, PR_CARD_W - 8, prViews[i].error ? prViews[i].error : "", prDim, PR_TEXT_SCALE, qtrue );

	// the skin, the sabers and the date under it
	Q_strncpyz( model, ps->values[PR_K_MODEL], sizeof( model ) );
	if ( slash )
		model[slash - ps->values[PR_K_MODEL]] = '\0';
	PR_Text( x + 4, y + 22 + PR_VIEW_H, PR_CARD_W - 8, va( "%s, skin %s", model, slash && slash[1] ? slash + 1 : "default" ), prWhite, 0.7f, qtrue );
	PR_Text( x + 4, y + 33 + PR_VIEW_H, PR_CARD_W - 8, PR_SaberText( ps ), prDim, 0.7f, qtrue );
	PR_Text( x + 4, y + 44 + PR_VIEW_H, PR_CARD_W - 8, ps->date, prDim, 0.65f, qtrue );

	// keeping it up to date, or out
	if ( PR_Button( x + 4, bottom - PR_CTRL_H - 3, 66, "Update", prPanelLight ) ) {
		const qboolean favorite = prPresets[i].favorite;

		PR_FromCurrent( &prPresets[i] );
		prPresets[i].favorite = favorite;
		PR_Save();
		PR_Message( va( "Updated %s" S_COLOR_WHITE " to who you are now", prPresets[i].values[PR_K_NAME] ) );
		return;
	}
	{
		const qboolean armed = (qboolean)( pr.confirm == i && cls.realtime - pr.confirmTime < PR_CONFIRM_MS );

		if ( PR_Button( x + PR_CARD_W - 70, bottom - PR_CTRL_H - 3, 66, armed ? "Sure?" : "Delete", armed ? prDanger : prPanelLight ) ) {
			if ( armed ) {
				char name[PR_VALUE_LEN];

				Q_strncpyz( name, ps->values[PR_K_NAME], sizeof( name ) );
				pr.confirm = -1;
				PR_Delete( i );
				PR_Message( va( "Deleted %s", name ) );
			} else {
				pr.confirm = i;
				pr.confirmTime = cls.realtime;
			}
			return;
		}
	}
	// anywhere else on it: become it
	if ( PR_Clicked( x, y, PR_CARD_W, PR_CARD_H ) ) {
		PR_Apply( i );
		PR_Close();
	}
}

// a card of a model or a skin: its character, its name, and below it what it is
static void PR_DrawViewCard( int slot, float x, float y, const char *value, const char *title, const char *under, qboolean current ) {
	const qboolean hover = PR_InRect( x, y, PR_CARD_W, PR_CARD_H );
	prView_t *v = &prSlotViews[slot];

	PR_Box( x, y, PR_CARD_W, PR_CARD_H, hover ? prCardHover : prCard );
	if ( current ) {
		PR_Fill( x, y, PR_CARD_W, 2, prAccent );
		PR_Fill( x, y + PR_CARD_H - 2, PR_CARD_W, 2, prAccent );
	}
	PR_Text( x + 4, y + 4, PR_CARD_W - 8, title, current ? prAccent : prWhite, 0.9f, qtrue );
	PR_BuildView( v, value, "", "" );
	if ( v->g2 )
		PR_RenderView( v, Cvar_VariableString( "char_color_red" ), Cvar_VariableString( "char_color_green" ), Cvar_VariableString( "char_color_blue" ),
			x + 2, y + 20, PR_CARD_W - 4, PR_VIEW_H, hover ? sinf( cls.realtime * 0.002f ) * 35.0f : 20.0f );
	else
		PR_Text( x + 4, y + 20 + PR_VIEW_H * 0.5f - 6, PR_CARD_W - 8, v->error ? v->error : "", prDim, PR_TEXT_SCALE, qtrue );
	PR_Text( x + 4, y + 22 + PR_VIEW_H, PR_CARD_W - 8, under, prDim, 0.7f, qtrue );
}

// the parts card: a head, a torso and legs, each stepped through, worn together when clicked
static void PR_DrawPartsCard( int slot, float x, float y ) {
	const char *names[3] = { prHeads[pr.head], prTorsos[pr.torso], prLowers[pr.lower] };
	const int counts[3] = { prNumHeads, prNumTorsos, prNumLowers };
	int *picks[3] = { &pr.head, &pr.torso, &pr.lower };
	char value[MAX_QPATH];

	Q_strncpyz( value, PR_PartsValue(), sizeof( value ) );
	PR_DrawViewCard( slot, x, y, value, "Put together", "", (qboolean)!Q_stricmp( Cvar_VariableString( "model" ), value ) );
	for ( int k = 0; k < 3; k++ ) {
		const float ry = y + 22 + PR_VIEW_H + k * ( PR_CTRL_H + 1 );

		if ( PR_Button( x + 4, ry, 18, "<", prPanelLight ) )
			*picks[k] = ( *picks[k] + counts[k] - 1 ) % counts[k];
		PR_Text( x + 24, ry + 2, PR_CARD_W - 48, va( "%s " S_COLOR_GREY "%i/%i", names[k], *picks[k] + 1, counts[k] ), prWhite, 0.7f, qtrue );
		if ( PR_Button( x + PR_CARD_W - 22, ry, 18, ">", prPanelLight ) )
			*picks[k] = ( *picks[k] + 1 ) % counts[k];
	}
	if ( PR_Clicked( x, y, PR_CARD_W, PR_CARD_H ) )
		PR_SetModel( PR_PartsValue() );
}

// what the page shows: the presets, a model's skins (the parts card first) or the models, as
// filtered; how many
static int PR_BuildShown( void ) {
	pr.numShown = 0;
	if ( pr.mode == PR_MODE_PRESETS ) {
		for ( int i = 0; i < prNumPresets; i++ ) {
			if ( PR_Matches( &prPresets[i] ) )
				pr.shown[pr.numShown++] = i;
		}
	} else if ( pr.mode == PR_MODE_SKINS ) {
		if ( PR_HasParts() && ( !pr.search[0] || Q_stristr( "put together parts head torso lower", pr.search ) ) )
			pr.shown[pr.numShown++] = -1;
		for ( int i = 0; i < prNumSkins; i++ ) {
			if ( !pr.search[0] || Q_stristr( prSkins[i], pr.search ) )
				pr.shown[pr.numShown++] = i;
		}
	} else {
		for ( int i = 0; i < prNumModels && pr.numShown < (int)ARRAY_LEN( pr.shown ); i++ ) {
			if ( !pr.search[0] || Q_stristr( prModels[i], pr.search ) )
				pr.shown[pr.numShown++] = i;
		}
	}
	return pr.numShown;
}

static void PR_DrawSlot( int slot, int index, float x, float y ) {
	if ( pr.mode == PR_MODE_PRESETS ) {
		PR_DrawCard( index, x, y );
		return;
	}
	if ( pr.mode == PR_MODE_SKINS ) {
		char value[MAX_QPATH];
		qboolean current;

		if ( index < 0 ) {
			PR_DrawPartsCard( slot, x, y );
			return;
		}
		Com_sprintf( value, sizeof( value ), "%s/%s", pr.skinModel, prSkins[index] );
		current = (qboolean)( !Q_stricmp( Cvar_VariableString( "model" ), value )
			|| ( !Q_stricmp( prSkins[index], "default" ) && !Q_stricmp( Cvar_VariableString( "model" ), pr.skinModel ) ) );
		PR_DrawViewCard( slot, x, y, value, prSkins[index], current ? "worn now" : "click to wear it", current );
		if ( PR_Clicked( x, y, PR_CARD_W, PR_CARD_H ) )
			PR_SetModel( !Q_stricmp( prSkins[index], "default" ) ? pr.skinModel : value );
		return;
	}
	{
		const char *name = prModels[index];
		const qboolean current = (qboolean)!Q_stricmp( PR_CurrentModel(), name );

		PR_DrawViewCard( slot, x, y, va( "%s/default", name ), name, "click for its skins", current );
		if ( PR_Button( x + 4, y + PR_CARD_H - PR_CTRL_H - 3, PR_CARD_W - 8, "Wear it", prPanelLight ) ) {
			PR_SetModel( name );
			return;
		}
		if ( PR_Clicked( x, y, PR_CARD_W, PR_CARD_H ) )
			PR_ShowSkins( name );
	}
}

void CL_Presets_Draw( void ) {
	const int perPage = PR_COLS * PR_ROWS;
	wgStyle_t style;
	int pages;

	if ( !pr.open )
		return;
	PR_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, prPanel );

	// the tabs: presets, the skins of a model, the models
	if ( PR_Button( 16, 12, 70, "Presets", pr.mode == PR_MODE_PRESETS ? prAccentFill : prPanelLight ) && pr.mode != PR_MODE_PRESETS )
		PR_Open( PR_MODE_PRESETS );
	if ( PR_Button( 90, 12, 120, pr.mode == PR_MODE_SKINS ? va( "Skins: %s", pr.skinModel ) : "Skins", pr.mode == PR_MODE_SKINS ? prAccentFill : prPanelLight )
		&& pr.mode != PR_MODE_SKINS )
		PR_ShowSkins( pr.skinModel[0] ? pr.skinModel : PR_CurrentModel() );
	if ( PR_Button( 214, 12, 80, "All models", pr.mode == PR_MODE_MODELS ? prAccentFill : prPanelLight ) && pr.mode != PR_MODE_MODELS )
		PR_ShowModels();
	if ( PR_Button( SCREEN_WIDTH - 16 - 60, 12, 60, "Close", prPanelLight ) ) {
		PR_Close();
		return;
	}
	if ( pr.mode == PR_MODE_PRESETS && PR_Button( SCREEN_WIDTH - 16 - 60 - 6 - 130, 12, 130, "Save who I am now", prPanelLight ) )
		PR_SaveCurrent();
	if ( pr.mode == PR_MODE_SKINS && PR_Button( SCREEN_WIDTH - 16 - 60 - 6 - 130, 12, 130, "Back to all models", prPanelLight ) )
		PR_ShowModels();

	// the search, and the favorites only
	style.font = pr.font;
	style.scale = PR_TEXT_SCALE;
	style.lineH = 13.0f;
	style.padX = 4.0f;
	style.padY = 2.0f;
	style.fill = prCardHover;
	style.border = prBorder;
	style.text = prWhite;
	style.dim = prDim;
	PR_Box( 16, 34, 200, PR_CTRL_H, pr.typing ? prCardHover : prPanelLight );
	WG_Field( &style, 16, 34, 200, PR_CTRL_H, pr.search, pr.typing, pr.mode == PR_MODE_PRESETS ? "search names, models, sabers" : "search", pr.cursorX, pr.cursorY );
	if ( PR_Clicked( 16, 34, 200, WG_FieldHeight( &style, 200, PR_CTRL_H, pr.search, pr.typing ) ) ) {
		if ( !pr.typing )
			WG_EditReset();
		pr.typing = qtrue;
		WG_FieldClick( pr.search, pr.clickX, pr.clickY );
	}
	if ( pr.search[0] && PR_Button( 220, 34, 18, "x", prPanelLight ) ) {
		pr.search[0] = '\0';
		pr.page = 0;
	}
	if ( pr.mode == PR_MODE_PRESETS ) {
		int favorites = 0;

		for ( int i = 0; i < prNumPresets; i++ )
			favorites += prPresets[i].favorite ? 1 : 0;
		if ( PR_Button( 244, 34, 100, va( "Favorites (%i)", favorites ), pr.favOnly ? prAccentFill : prPanelLight ) ) {
			pr.favOnly = (qboolean)!pr.favOnly;
			pr.page = 0;
		}
		PR_Text( 352, 36, SCREEN_WIDTH - 368, "Click a character to become it; * makes it a favorite", prDim, PR_TEXT_SCALE, qfalse );
	} else if ( pr.mode == PR_MODE_SKINS ) {
		PR_Text( 244, 36, SCREEN_WIDTH - 260, PR_HasParts() ? va( "%i skins and parts to put together: click one to wear it", prNumSkins )
			: va( "%i skins: click one to wear it", prNumSkins ), prDim, PR_TEXT_SCALE, qfalse );
	} else {
		PR_Text( 244, 36, SCREEN_WIDTH - 260, va( "%i models: click one for its skins, or wear it as it is", prNumModels ), prDim, PR_TEXT_SCALE, qfalse );
	}

	PR_BuildShown();
	pages = PR_Pages();
	pr.page = Com_Clampi( 0, pages - 1, pr.page );
	if ( !pr.numShown ) {
		const char *none = pr.search[0] || pr.favOnly ? "None match" : pr.mode == PR_MODE_PRESETS
			? "No presets yet: Save who I am now keeps your name, model, skin and sabers as one" : pr.mode == PR_MODE_SKINS ? "This model has no skins here" : "No models here";

		PR_Text( 16, 200, SCREEN_WIDTH - 32, none, prDim, 1.0f, qtrue );
	}
	for ( int slot = 0; slot < perPage; slot++ ) {
		const int n = pr.page * perPage + slot;

		if ( n >= pr.numShown )
			break;
		PR_DrawSlot( slot, pr.shown[n], PR_GRID_X + ( slot % PR_COLS ) * ( PR_CARD_W + PR_CARD_GAP ), PR_GRID_Y + ( slot / PR_COLS ) * ( PR_CARD_H + PR_CARD_GAP ) );
		if ( !pr.open )
			return;
		// the list changed under it
		if ( pr.mode == PR_MODE_PRESETS && pr.numShown != PR_BuildShown() )
			break;
	}
	// the pages: first, back, where, on, last
	if ( pages > 1 ) {
		const float y = PR_GRID_Y + PR_ROWS * ( PR_CARD_H + PR_CARD_GAP ) + 2, cx = SCREEN_WIDTH * 0.5f;

		if ( PR_Button( cx - 150, y, 36, "<<", pr.page > 0 ? prPanelLight : prCard ) )
			pr.page = 0;
		if ( PR_Button( cx - 110, y, 50, "<", pr.page > 0 ? prPanelLight : prCard ) )
			pr.page = Q_max( 0, pr.page - 1 );
		PR_Text( cx - 55, y + 2, 110, va( "page %i of %i  (%i)", pr.page + 1, pages, pr.numShown ), prDim, PR_TEXT_SCALE, qtrue );
		if ( PR_Button( cx + 60, y, 50, ">", pr.page < pages - 1 ? prPanelLight : prCard ) )
			pr.page = Q_min( pages - 1, pr.page + 1 );
		if ( PR_Button( cx + 114, y, 36, ">>", pr.page < pages - 1 ? prPanelLight : prCard ) )
			pr.page = pages - 1;
	}
	if ( pr.message[0] && cls.realtime - pr.messageTime < PR_MESSAGE_MS )
		PR_ColorText( 16, 458, SCREEN_WIDTH - 32, pr.message, PR_TEXT_SCALE, qfalse );
	// a click anywhere else leaves the search
	if ( pr.click )
		pr.typing = qfalse;
	pr.click = qfalse;
	WG_DrawOverlay();
	re->SetColor( NULL );
	re->DrawStretchPic( pr.cursorX, pr.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}
