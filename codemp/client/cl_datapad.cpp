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

// cl_datapad.cpp -- the datapad: held up like a phone, or folded in the corner
//
// Open, it rises in the bottom right corner, from above the force and ammo HUD down to the screen's
// edge, and the mouse works on it; Esc puts it away. An app that needs the room has it bigger, on
// the right side of the screen. Folded, it sits in the corner above the HUD while you play, taking
// neither keys nor the mouse. On a wide screen with cl_ratioFix it keeps its shape, as the text does.
//
// Its screen has apps: the home screen lists them, one opens over it and Back returns. Settings
// is the first: its colour, the keys that open and close it, and whether your character holds a
// datapad (RPMod's /emlift) while it's open; a moment's crouch ends that, as C does.
//
//   datapad              opens it, or puts it away
//   datapad open|close
//   datapad settings     open, on Settings
//   datapad pos <x> <y>  where it opens, on the 640x480 screen; datapad pos alone: back above the
//                        HUD (cl_datapadX, cl_datapadY)
//
// Folded (datapad fold): the same in the corner, small, while you play on. Not offered for now.

#include "client.h"
#include "cl_datapad.h"
#include "cl_widgets.h"

#define DP_CMD			"datapad"
#define DP_OPEN_CMD		DP_CMD " open"
#define DP_CLOSE_CMD	DP_CMD " close"
#define DP_SLIDE_MS		220
#define DP_APP_MS		200			// an app sliding in over the home screen, and out
#define DP_EMOTE		"emlift"
#define DP_EMOTE_END_MS	150			// the crouch that ends it

// folded: above the force and ammo HUD in the bottom right corner; open, from there down to the
// screen's edge (widths on a 4:3 screen: narrower on a wide one with cl_ratioFix)
#define DP_SMALL_W		172.0f
#define DP_SMALL_H		100.0f
#define DP_HUD_H		118.0f		// the HUD's height in the corner, kept clear
#define DP_MARGIN		6.0f

// an app that needs the room: the right side of the screen, down to its bottom edge
#define DP_BIG_W		272.0f
#define DP_BIG_TOP		26.0f

#define DP_TEXT_SCALE	0.8f

typedef enum {
	DP_CLOSED,
	DP_FOLDED,
	DP_OPEN
} dpState_t;

typedef enum {
	DP_HOME,
	DP_SETTINGS,
	DP_PAZAAK,
	DP_HOLONET,
	DP_COMPORT,
	DP_NUM_APPS
} dpApp_t;

// an app that needs the room has the datapad bigger while it's on its screen
static const qboolean dpAppFull[DP_NUM_APPS] = { qfalse, qfalse, qtrue, qtrue, qtrue };

// the apps in files of their own: what each is told and given
static void DP_AppShown( dpApp_t app, qboolean shown ) {
	if ( app == DP_PAZAAK )
		CL_Pazaak_Shown( shown );
	else if ( app == DP_HOLONET )
		CL_HoloNet_Shown( shown );
	else if ( app == DP_COMPORT )
		CL_Comport_Shown( shown );
}

typedef enum {
	DP_BIND_NONE,
	DP_BIND_OPEN,
	DP_BIND_CLOSE
} dpBind_t;

// the screen's colour, picked in Settings: everything lit on it is drawn in it
typedef struct {
	const char	*name;
	float		r, g, b;
} dpColor_t;

static const dpColor_t dpColors[] = {
	{ "Cyan", 0.25f, 0.85f, 0.95f },
	{ "Blue", 0.35f, 0.55f, 1.0f },
	{ "Green", 0.35f, 0.95f, 0.45f },
	{ "Amber", 1.0f, 0.68f, 0.15f },
	{ "Red", 1.0f, 0.32f, 0.28f },
	{ "Violet", 0.75f, 0.45f, 1.0f },
	{ "White", 0.88f, 0.90f, 0.92f },
};
#define DP_NUM_COLORS	( (int)ARRAY_LEN( dpColors ) )

static const float dpCase[4]		= { 0.10f, 0.11f, 0.13f, 1.0f };
static const float dpCaseEdge[4]	= { 0.36f, 0.38f, 0.42f, 1.0f };
static const float dpCaseDark[4]	= { 0.04f, 0.04f, 0.05f, 1.0f };
static const float dpLedOn[4]		= { 0.35f, 1.0f, 0.55f, 1.0f };
static const float dpLedBlink[4]	= { 0.95f, 0.65f, 0.15f, 1.0f };
const float dpWhite[4]				= { 1.0f, 1.0f, 1.0f, 1.0f };

// the lit colours of the moment, from the one picked
float dpGlow[4], dpScan[4], dpText[4], dpDim[4], dpScreen[4], dpButton[4], dpButtonHover[4], dpTile[4];

static struct {
	dpState_t	state;
	dpRect_t	from;				// where it was drawn as it began going to where state puts it
	int			moveTime;
	qboolean	drawn;				// drawn last frame: from is a real place
	dpRect_t	last;

	dpApp_t		app;				// the screen's: the home screen or an app over it
	dpApp_t		appFrom;
	int			appTime;
	dpBind_t	binding;			// a key being picked in Settings
	qboolean	emoting;			// the emote's on: ended as it folds or closes

	int			font, titleFont;
	sfxHandle_t	openSound, closeSound, clickSound;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	char		message[128];
	int			messageTime;
} dp;

static cvar_t *cl_datapadX, *cl_datapadY, *cl_datapadColor, *cl_datapadEmote;

/*
===============================================================================

Where it is

===============================================================================
*/

// a width on a 4:3 screen as it's drawn here: narrower on a wide screen with cl_ratioFix, so it
// keeps its shape as the text does
float DP_Wide( float w ) {
	return w * ( cls.widthRatioCoef > 0.0f ? cls.widthRatioCoef : 1.0f );
}

static void DP_FoldedRect( dpRect_t *r ) {
	r->w = DP_Wide( DP_SMALL_W );
	r->h = DP_SMALL_H;
	r->x = cl_datapadX->value >= 0 ? cl_datapadX->value : SCREEN_WIDTH - r->w - DP_Wide( DP_MARGIN );
	r->y = cl_datapadY->value >= 0 ? cl_datapadY->value : SCREEN_HEIGHT - DP_HUD_H - DP_SMALL_H - DP_MARGIN;
	r->x = Com_Clamp( 0, SCREEN_WIDTH - r->w, r->x );
	r->y = Com_Clamp( 0, SCREEN_HEIGHT - r->h, r->y );
}

// open: where it's folded, from its top down past the screen's bottom edge, held up from below
static void DP_PhoneRect( dpRect_t *r ) {
	DP_FoldedRect( r );
	r->h = SCREEN_HEIGHT - r->y + 4;
}

// for an app that needs the room: the right side of the screen
static void DP_FullRect( dpRect_t *r ) {
	r->w = DP_Wide( DP_BIG_W );
	r->x = SCREEN_WIDTH - r->w - DP_Wide( DP_MARGIN * 2 );
	r->y = DP_BIG_TOP;
	r->h = SCREEN_HEIGHT - DP_BIG_TOP + 4;
}

static void DP_OpenRect( dpRect_t *r ) {
	if ( dpAppFull[dp.app] )
		DP_FullRect( r );
	else
		DP_PhoneRect( r );
}

// where the state puts it; closed, below the screen where it was
static void DP_Target( dpRect_t *r ) {
	if ( dp.state == DP_OPEN ) {
		DP_OpenRect( r );
	} else if ( dp.state == DP_FOLDED ) {
		DP_FoldedRect( r );
	} else {
		*r = dp.from;
		r->y = SCREEN_HEIGHT + 4;
	}
}

static float DP_Ease( int since, int ms ) {
	const float t = Com_Clamp( 0.0f, 1.0f, since / (float)ms );

	return t * t * ( 3.0f - 2.0f * t );
}

// where it's drawn now, on its way from where it was; false once it's gone
static qboolean DP_Rect( dpRect_t *r ) {
	dpRect_t to;
	const float t = DP_Ease( cls.realtime - dp.moveTime, DP_SLIDE_MS );

	DP_Target( &to );
	if ( dp.state == DP_CLOSED && t >= 1.0f )
		return qfalse;
	r->x = dp.from.x + ( to.x - dp.from.x ) * t;
	r->y = dp.from.y + ( to.y - dp.from.y ) * t;
	r->w = dp.from.w + ( to.w - dp.from.w ) * t;
	r->h = dp.from.h + ( to.h - dp.from.h ) * t;
	return qtrue;
}

/*
===============================================================================

Open, folded, closed

===============================================================================
*/

static void DP_Register( void ) {
	dp.font = re->RegisterFont( "arialnb" );
	if ( !dp.font )
		dp.font = cls.menuFont;
	dp.titleFont = re->RegisterFont( "ergoec" );
	if ( !dp.titleFont )
		dp.titleFont = dp.font;
	dp.openSound = S_RegisterSound( "sound/interface/button1.wav" );
	dp.closeSound = S_RegisterSound( "sound/interface/esc.wav" );
	dp.clickSound = S_RegisterSound( "sound/interface/button1.wav" );
}

static void DP_Sound( sfxHandle_t sound ) {
	if ( sound )
		S_StartLocalSound( sound, CHAN_LOCAL_SOUND );
}

// for the apps
void DP_PlaySound( sfxHandle_t sound ) {
	DP_Sound( sound );
}

int DP_Font( void ) {
	return dp.font;
}

int DP_TitleFont( void ) {
	return dp.titleFont;
}

float DP_CursorX( void ) {
	return dp.cursorX;
}

float DP_CursorY( void ) {
	return dp.cursorY;
}

static void DP_Message( const char *text ) {
	Q_strncpyz( dp.message, text, sizeof( dp.message ) );
	dp.messageTime = cls.realtime;
}

// the character holds a datapad while it's open: RPMod's emote, ended by a moment's crouch
static void DP_Emote( qboolean on ) {
	if ( on ) {
		if ( dp.emoting || !cl_datapadEmote->integer || cls.state != CA_ACTIVE || cl.snap.ps.pm_type == PM_SPECTATOR
			|| cl.snap.ps.stats[STAT_HEALTH] <= 0 )
			return;
		dp.emoting = qtrue;
		CL_AddReliableCommand( DP_EMOTE, qfalse );
	} else if ( dp.emoting ) {
		dp.emoting = qfalse;
		if ( cls.state == CA_ACTIVE )
			CL_TapCrouch( DP_EMOTE_END_MS );
	}
}

static void DP_Set( dpState_t state ) {
	dpRect_t r;

	if ( state == dp.state )
		return;
	if ( state != DP_CLOSED && ( cls.state != CA_ACTIVE || !cls.cgameStarted ) ) {
		Com_Printf( "Datapad: join a server first\n" );
		return;
	}
	if ( !dp.font )
		DP_Register();
	// from where it's drawn now; from below the screen when it wasn't
	if ( dp.drawn && cls.realtime - dp.moveTime >= 0 ) {
		dp.from = dp.last;
	} else {
		state == DP_FOLDED ? DP_FoldedRect( &dp.from ) : DP_OpenRect( &dp.from );
		dp.from.y = SCREEN_HEIGHT + 4;
	}
	dp.moveTime = cls.realtime;
	DP_Sound( state == DP_CLOSED || ( state == DP_FOLDED && dp.state == DP_OPEN ) ? dp.closeSound : dp.openSound );
	dp.state = state;
	dp.click = qfalse;
	dp.binding = DP_BIND_NONE;
	if ( state == DP_OPEN ) {
		// the tools give way; the mouse starts on its screen, as a phone's held up
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
		CL_SpecialMoves_Close();
		CL_AssetManager_Close();
		CL_ChatLog_Close();
		CL_RadialMenu_CloseSettings();
		CL_Languages_Close();
		DP_OpenRect( &r );
		dp.cursorX = r.x + r.w * 0.5f;
		dp.cursorY = r.y + ( SCREEN_HEIGHT - r.y ) * 0.4f;
		Key_SetCatcher( Key_GetCatcher() | KEYCATCH_DATAPAD );
		DP_Emote( qtrue );
	} else {
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_DATAPAD );
		DP_Emote( qfalse );
		DP_AppShown( dp.app, qfalse );
		// put away: on the home screen next time
		if ( state == DP_CLOSED )
			dp.app = dp.appFrom = DP_HOME;
	}
}

void CL_Datapad_Close( void ) {
	if ( dp.state == DP_OPEN )
		DP_Set( DP_FOLDED );
}

void CL_Datapad_Disconnect( void ) {
	// on the next server as it was, but never holding the keys
	dp.emoting = qfalse;
	dp.binding = DP_BIND_NONE;
	if ( dp.state == DP_OPEN ) {
		dp.state = DP_FOLDED;
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_DATAPAD );
	}
	dp.drawn = qfalse;
}

static void DP_SetApp( dpApp_t app ) {
	if ( app == dp.app )
		return;
	// to or from an app that needs the room: the datapad grows or shrinks to it
	if ( dpAppFull[app] != dpAppFull[dp.app] && dp.drawn ) {
		dp.from = dp.last;
		dp.moveTime = cls.realtime;
	}
	dp.appFrom = dp.app;
	dp.app = app;
	dp.appTime = cls.realtime;
	dp.binding = DP_BIND_NONE;
	DP_AppShown( dp.appFrom, qfalse );
	DP_AppShown( app, qtrue );
}

/*
===============================================================================

Drawing

===============================================================================
*/

static void DP_Colors( void ) {
	const dpColor_t *c = &dpColors[Com_Clampi( 0, DP_NUM_COLORS - 1, cl_datapadColor->integer )];
	const float v[3] = { c->r, c->g, c->b };

	for ( int i = 0; i < 3; i++ ) {
		dpGlow[i] = v[i];
		dpScan[i] = v[i];
		dpText[i] = 0.55f + v[i] * 0.45f;
		dpDim[i] = v[i] * 0.66f;
		dpScreen[i] = v[i] * 0.09f;
		dpButton[i] = v[i] * 0.22f;
		dpButtonHover[i] = v[i] * 0.38f;
		dpTile[i] = v[i] * 0.16f;
	}
	dpGlow[3] = 0.55f;
	dpScan[3] = 0.05f;
	dpText[3] = dpDim[3] = 1.0f;
	dpScreen[3] = 0.93f;
	dpButton[3] = dpButtonHover[3] = 0.95f;
	dpTile[3] = 0.9f;
}

void DP_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

void DP_Frame( float x, float y, float w, float h, const float *color ) {
	DP_Fill( x, y, w, 1, color );
	DP_Fill( x, y + h - 1, w, 1, color );
	DP_Fill( x, y, 1, h, color );
	DP_Fill( x + w - 1, y, 1, h, color );
}

// text with colour codes in it, RPMod's too (^a-^z, ^A-^Z on RPMod, as the console shows them): each
// run in its colour, at the alpha it's given; too long, cut with ... at the end
#define DP_MAX_RUNS		32

static void DP_ColoredText( float x, float y, float w, const char *text, const float *color, int font, float scale, qboolean centred ) {
	char runs[DP_MAX_RUNS][MAX_STRING_CHARS / 4];
	float colors[DP_MAX_RUNS][4], total = 0;
	int n = 0, len = 0;

	memcpy( colors[0], color, sizeof( colors[0] ) );
	runs[0][0] = '\0';
	for ( const char *p = text; *p; p++ ) {
		const float *c = Con_TypedColor( p );

		if ( c ) {
			// a new colour: a new run, unless this one's still empty
			if ( len && n < DP_MAX_RUNS - 1 ) {
				runs[n][len] = '\0';
				n++;
				len = 0;
			}
			memcpy( colors[n], c, sizeof( float ) * 3 );
			colors[n][3] = color[3];
			p++;
			continue;
		}
		if ( len < (int)sizeof( runs[0] ) - 4 )
			runs[n][len++] = *p;
	}
	runs[n][len] = '\0';
	n++;

	for ( int i = 0; i < n; i++ )
		total += re->Font_StrLenPixels( runs[i], font, scale );
	// too long: the end cut off, ... in its place
	if ( total > w ) {
		const float dots = re->Font_StrLenPixels( "...", font, scale );

		while ( n > 0 && total + dots > w ) {
			int l = (int)strlen( runs[n - 1] );

			if ( !l ) {
				n--;
				continue;
			}
			total -= re->Font_StrLenPixels( runs[n - 1], font, scale );
			runs[n - 1][l - 1] = '\0';
			total += re->Font_StrLenPixels( runs[n - 1], font, scale );
		}
		if ( n > 0 ) {
			Q_strcat( runs[n - 1], sizeof( runs[0] ), "..." );
			total += dots;
		}
	}
	if ( centred )
		x += ( w - total ) * 0.5f;
	for ( int i = 0; i < n; i++ ) {
		re->Font_DrawString( (int)x, (int)y, runs[i], colors[i], font, -1, scale );
		x += re->Font_StrLenPixels( runs[i], font, scale );
	}
}

void DP_Text( float x, float y, float w, const char *text, const float *color, int font, float scale, qboolean centred ) {
	char buf[MAX_STRING_CHARS];
	int len;

	// colour codes in it: drawn as colours, RPMod's too, never as text
	for ( const char *p = text; *p; p++ ) {
		if ( p[0] == Q_COLOR_ESCAPE && p[1] && Con_TypedColor( p ) ) {
			DP_ColoredText( x, y, w, text, color, font, scale, centred );
			return;
		}
	}
	Q_strncpyz( buf, text, sizeof( buf ) );
	len = (int)strlen( buf );
	while ( len > 3 && re->Font_StrLenPixels( buf, font, scale ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	if ( centred )
		x += ( w - re->Font_StrLenPixels( buf, font, scale ) ) * 0.5f;
	re->Font_DrawString( (int)x, (int)y, buf, color, font, -1, scale );
}

qboolean DP_InRect( float x, float y, float w, float h ) {
	return (qboolean)( dp.cursorX >= x && dp.cursorX < x + w && dp.cursorY >= y && dp.cursorY < y + h );
}

// a click in the rect, taken
qboolean DP_Clicked( float x, float y, float w, float h ) {
	if ( !dp.click || dp.clickX < x || dp.clickX >= x + w || dp.clickY < y || dp.clickY >= y + h )
		return qfalse;
	dp.click = qfalse;
	DP_Sound( dp.clickSound );
	return qtrue;
}

qboolean DP_Button( float x, float y, float w, const char *label, qboolean on ) {
	DP_Fill( x, y, w, DP_CTRL_H, on || DP_InRect( x, y, w, DP_CTRL_H ) ? dpButtonHover : dpButton );
	DP_Frame( x, y, w, DP_CTRL_H, dpGlow );
	DP_Text( x, y + 3, w, label, dpText, dp.font, 0.62f, qtrue );
	return DP_Clicked( x, y, w, DP_CTRL_H );
}

/*
===============================================================================

Web pages' blocks, for the apps that read them (HoloNet, Comport)

===============================================================================
*/

static void DP_BlockStyle( wgStyle_t *style, float scale ) {
	memset( style, 0, sizeof( *style ) );
	style->font = dp.font;
	style->scale = scale;
	style->lineH = 11.0f * scale / 0.6f;
	style->text = dpText;
	style->dim = dpDim;
}

// a picture's height drawn w wide, at its own shape on the screen
static float DP_PictureHeight( float w, int pw, int ph ) {
	const float sx = cls.glconfig.vidWidth / (float)SCREEN_WIDTH, sy = cls.glconfig.vidHeight / (float)SCREEN_HEIGHT;

	if ( pw <= 0 || ph <= 0 || sy <= 0 )
		return 60.0f;
	return Q_min( 280.0f, w * sx / sy * ph / pw );
}

float DP_DrawBlocks( const std::vector<htmlBlock_t> &blocks, float x, float y, float w, float top, float bottom ) {
	const float start = y;
	wgStyle_t body, head;

	DP_BlockStyle( &body, 0.6f );
	DP_BlockStyle( &head, 0.72f );
	for ( size_t i = 0; i < blocks.size(); i++ ) {
		const htmlBlock_t &b = blocks[i];
		const float indent = DP_Wide( 10.0f ) * b.depth, bx = x + indent, bw = w - indent;
		float h = 0;

		switch ( b.kind ) {
		case HB_IMAGE: {
			int pw, ph;
			const qhandle_t pic = HTML_Picture( b.src.c_str(), &pw, &ph );
			const float ih = pic ? DP_PictureHeight( bw, pw, ph ) : 40.0f;
			const float y0 = Q_max( y, top ), y1 = Q_min( y + ih, bottom );

			if ( y1 > y0 ) {
				if ( pic ) {
					re->SetColor( NULL );
					re->DrawStretchPic( bx, y0, bw, y1 - y0, 0, ( y0 - y ) / ih, 1, ( y1 - y ) / ih, pic );
				} else {
					DP_Fill( bx, y0, bw, y1 - y0, dpButton );
					if ( y + 14 <= bottom && y >= top )
						DP_Text( bx, y + 14, bw, b.text.empty() ? "[picture]" : va( "[picture: %s]", b.text.c_str() ), dpDim, dp.font, 0.55f, qtrue );
				}
			}
			h = ih + 6;
			break;
		}
		case HB_RULE:
			if ( y + 3 >= top && y + 3 <= bottom )
				DP_Fill( bx, y + 3, bw, 1, dpGlow );
			h = 8;
			break;
		default: {
			const wgStyle_t *st = b.kind == HB_HEADING ? &head : &body;
			const char *prefix = b.kind == HB_LIST ? "- " : "";
			const std::string text = prefix + b.text;
			const float *color = b.kind == HB_HEADING ? dpWhite : b.kind == HB_QUOTE_HEAD || b.kind == HB_CODE ? dpDim : dpText;
			const int lines = WG_TextLines( st, text.c_str(), bw );
			int first = 0, count = lines;

			// only the lines on the screen
			while ( first < lines && y + first * st->lineH < top )
				first++;
			while ( count > first && y + count * st->lineH > bottom )
				count--;
			if ( count > first )
				WG_DrawText( st, bx, y + first * st->lineH, bw, text.c_str(), first, count - first, color );
			h = lines * st->lineH + ( b.kind == HB_HEADING ? 6 : b.kind == HB_LIST ? 1 : 5 );
			break;
		}
		}
		// the quote's bars beside it
		for ( int d = 0; d < b.depth; d++ ) {
			const float qx = x + DP_Wide( 10.0f ) * d + DP_Wide( 3 ), y0 = Q_max( y, top ), y1 = Q_min( y + h - 2, bottom );

			if ( y1 > y0 )
				DP_Fill( qx, y0, DP_Wide( 2 ), y1 - y0, dpDim );
		}
		y += h;
	}
	return y - start;
}

void DP_ScrollBar( float x, float y, float h, float scroll, float content ) {
	if ( content <= h || content <= 0 )
		return;
	DP_Fill( x, y, DP_Wide( 3 ), h, dpButton );
	DP_Fill( x, y + h * scroll / content, DP_Wide( 3 ), Q_max( 8.0f, h * h / content ), dpGlow );
}

// the device: a case with a header and its lights, and the screen's place in it
static void DP_DrawCase( const dpRect_t *r, float big, dpRect_t *screen ) {
	const float head = 14.0f + 6.0f * big, side = DP_Wide( 6.0f );
	// room along the bottom for its buttons once it's open
	const float foot = 10.0f + 20.0f * Q_min( 1.0f, big * 2.0f );
	const qboolean blink = (qboolean)( ( cls.realtime / 700 ) & 1 );

	DP_Fill( r->x, r->y, r->w, r->h, dpCase );
	DP_Frame( r->x, r->y, r->w, r->h, dpCaseEdge );
	DP_Frame( r->x + 1, r->y + 1, r->w - 2, r->h - 2, dpCaseDark );
	DP_Text( r->x + side, r->y + 2 + 2 * big, r->w * 0.6f, "DATAPAD", dpDim, dp.titleFont, 0.42f + 0.18f * big, qfalse );
	DP_Fill( r->x + r->w - side - DP_Wide( 5 ), r->y + 5 + 2 * big, DP_Wide( 4 ), 4, dpLedOn );
	DP_Fill( r->x + r->w - side - DP_Wide( 12 ), r->y + 5 + 2 * big, DP_Wide( 4 ), 4, blink ? dpLedBlink : dpCaseDark );

	screen->x = r->x + side;
	screen->y = r->y + head;
	screen->w = r->w - side * 2;
	screen->h = r->h - head - foot;
	DP_Fill( screen->x, screen->y, screen->w, screen->h, dpScreen );
	for ( float y = screen->y + 2; y < screen->y + screen->h - 1; y += 3 )
		DP_Fill( screen->x + 1, y, screen->w - 2, 1, dpScan );
	DP_Frame( screen->x, screen->y, screen->w, screen->h, dpGlow );
}

// the bar at the top of the screen: the time here, and where you are
// the day of the week, 0 Sunday, of a date (Sakamoto's)
static int DP_Weekday( int y, int m, int d ) {
	static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

	if ( m < 3 )
		y--;
	return ( y + y / 4 - y / 100 + y / 400 + t[m - 1] + d ) % 7;
}

// the n-th Sunday of a month (n 1 the first)
static int DP_Sunday( int y, int m, int n ) {
	return 1 + ( 7 - DP_Weekday( y, m, 1 ) ) % 7 + ( n - 1 ) * 7;
}

// the time in New York now, whatever this computer's own zone: UTC-5, UTC-4 from the second Sunday
// of March at 2:00 to the first Sunday of November at 2:00 (1:00 in standard time)
static void DP_NewYork( struct tm *out ) {
	const time_t utc = time( NULL );
	time_t t = utc - 5 * 3600;
	struct tm *st = gmtime( &t );
	int start, end, mdhm;

	if ( !st ) {
		memset( out, 0, sizeof( *out ) );
		return;
	}
	*out = *st;
	start = DP_Sunday( out->tm_year + 1900, 3, 2 );
	end = DP_Sunday( out->tm_year + 1900, 11, 1 );
	// month, day, hour as one number to compare: standard time's
	mdhm = ( out->tm_mon + 1 ) * 10000 + out->tm_mday * 100 + out->tm_hour;
	if ( mdhm >= 3 * 10000 + start * 100 + 2 && mdhm < 11 * 10000 + end * 100 + 1 ) {
		t += 3600;
		st = gmtime( &t );
		if ( st )
			*out = *st;
	}
}

// JEDI's own time: <year>.<day> ABY - <hour><minute>/<seconds> GST, New York's time. The year goes
// up one a month: 474 in November 2026; the day is the month's
static const char *DP_JediTime( void ) {
	struct tm ny;
	int year;

	DP_NewYork( &ny );
	year = 474 + ( ny.tm_year + 1900 - 2026 ) * 12 + ( ny.tm_mon + 1 - 11 );
	return va( "%i.%02i ABY - %02i%02i/%02i GST", year, ny.tm_mday, ny.tm_hour, ny.tm_min, ny.tm_sec );
}

static void DP_StatusBar( const dpRect_t *s, float scale ) {
	const char *when = DP_JediTime(), *server;
	const float tw = re->Font_StrLenPixels( when, dp.font, scale ) + DP_Wide( 8 );

	DP_Text( s->x + 4, s->y + 2, s->w - 8, when, dpText, dp.font, scale, qfalse );
	server = Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], "sv_hostname" );
	if ( server[0] && s->w - tw > DP_Wide( 60 ) )
		DP_Text( s->x + 4 + tw, s->y + 2, s->w - tw - 8, server, dpDim, dp.font, scale, qfalse );
	DP_Fill( s->x + 2, s->y + DP_STATUS_H - 1, s->w - 4, 1, dpGlow );
}

// an app's icon: a lit tile (square on the screen, wide or not), its picture, its name under it; clicked
static qboolean DP_AppIcon( float x, float y, float size, const char *name, void ( *picture )( float, float, float, float ) ) {
	const float w = DP_Wide( size );
	const qboolean hover = DP_InRect( x, y, w, size + 13 );

	DP_Fill( x, y, w, size, hover ? dpButtonHover : dpTile );
	DP_Frame( x, y, w, size, dpGlow );
	picture( x, y, w, size );
	DP_Text( x - w * 0.3f, y + size + 2, w * 1.6f, name, hover ? dpWhite : dpText, dp.font, 0.6f, qtrue );
	return DP_Clicked( x, y, w, size + 13 );
}

// Settings' picture: three sliders
static void DP_SettingsPicture( float x, float y, float w, float h ) {
	const float knob[3] = { 0.30f, 0.68f, 0.45f };

	for ( int i = 0; i < 3; i++ ) {
		const float ly = y + h * ( 0.28f + 0.22f * i );

		DP_Fill( x + w * 0.2f, ly, w * 0.6f, 2, dpText );
		DP_Fill( x + w * ( 0.2f + 0.6f * knob[i] ) - DP_Wide( 3 ), ly - 3, DP_Wide( 6 ), 8, dpWhite );
	}
}

// Pazaak's picture: two cards, fanned
static void DP_PazaakPicture( float x, float y, float w, float h ) {
	const float back[4] = { 0.25f, 0.45f, 0.95f, 1.0f }, front[4] = { 0.85f, 0.30f, 0.28f, 1.0f };

	DP_Fill( x + w * 0.22f, y + h * 0.18f, w * 0.36f, h * 0.56f, back );
	DP_Frame( x + w * 0.22f, y + h * 0.18f, w * 0.36f, h * 0.56f, dpWhite );
	DP_Fill( x + w * 0.42f, y + h * 0.28f, w * 0.36f, h * 0.56f, front );
	DP_Frame( x + w * 0.42f, y + h * 0.28f, w * 0.36f, h * 0.56f, dpWhite );
}

// the HoloNet's picture: an antenna sending
static void DP_HoloNetPicture( float x, float y, float w, float h ) {
	DP_Fill( x + w * 0.47f, y + h * 0.35f, w * 0.06f, h * 0.45f, dpWhite );
	DP_Fill( x + w * 0.32f, y + h * 0.78f, w * 0.36f, h * 0.06f, dpWhite );
	for ( int i = 1; i <= 3; i++ ) {
		const float r = 0.08f * i;

		DP_Fill( x + w * ( 0.5f - r - 0.04f ), y + h * ( 0.32f - r * 0.6f ), w * 0.04f, h * ( 0.1f + r * 0.8f ), dpText );
		DP_Fill( x + w * ( 0.5f + r ), y + h * ( 0.32f - r * 0.6f ), w * 0.04f, h * ( 0.1f + r * 0.8f ), dpText );
	}
}

// the Comport's picture: two speech bubbles
static void DP_ComportPicture( float x, float y, float w, float h ) {
	DP_Fill( x + w * 0.16f, y + h * 0.2f, w * 0.48f, h * 0.32f, dpText );
	DP_Fill( x + w * 0.22f, y + h * 0.52f, w * 0.1f, h * 0.1f, dpText );
	DP_Fill( x + w * 0.38f, y + h * 0.48f, w * 0.46f, h * 0.3f, dpWhite );
	DP_Fill( x + w * 0.7f, y + h * 0.78f, w * 0.1f, h * 0.1f, dpWhite );
}

static void DP_DrawHome( const dpRect_t *s ) {
	const float size = 34.0f;
	const float x = s->x + DP_Wide( 10 ), y = s->y + DP_STATUS_H + 10;

	// the apps in their order, three to a row
	static const struct {
		dpApp_t		app;
		const char	*name;
		void		( *picture )( float, float, float, float );
	} apps[] = {
		{ DP_HOLONET,	"HoloNet",	DP_HoloNetPicture },
		{ DP_COMPORT,	"Comport",	DP_ComportPicture },
		{ DP_PAZAAK,	"Pazaak",	DP_PazaakPicture },
		{ DP_SETTINGS,	"Settings",	DP_SettingsPicture },
	};

	for ( int i = 0; i < (int)ARRAY_LEN( apps ); i++ ) {
		if ( DP_AppIcon( x + DP_Wide( size + 16 ) * ( i % 3 ), y + ( size + 26 ) * ( i / 3 ), size, apps[i].name, apps[i].picture ) )
			DP_SetApp( apps[i].app );
	}
}

// a key bound to the command, or "none"
static const char *DP_KeyFor( const char *command ) {
	const int key = Key_GetKey( command );

	return key >= 0 ? Key_KeynumToString( key ) : "none";
}

// every key bound to the command let go: one key at a time does it
static void DP_Unbind( const char *command ) {
	for ( int key; ( key = Key_GetKey( command ) ) >= 0; )
		Key_SetBinding( key, "" );
}

// laid out in parts of its width: it fits the corner's datapad, narrow on a wide screen or not
static void DP_DrawSettings( const dpRect_t *s ) {
	const float pad = DP_Wide( 6 ), x = s->x + pad, w = s->w - pad * 2;
	const float small = 0.6f, label = w * 0.44f;
	float y = s->y + DP_STATUS_H + 4;

	if ( DP_Button( x, y, w * 0.36f, "< Back", qfalse ) ) {
		DP_SetApp( DP_HOME );
		return;
	}
	DP_Text( x + w * 0.4f, y + 1, w * 0.6f, "Settings", dpText, dp.titleFont, 0.5f, qfalse );
	y += DP_CTRL_H + 8;

	// the colour it's lit in
	DP_Text( x, y, w, va( "Colour: %s", dpColors[Com_Clampi( 0, DP_NUM_COLORS - 1, cl_datapadColor->integer )].name ), dpDim, dp.font, small, qfalse );
	y += 12;
	{
		const float gap = DP_Wide( 3 ), sw = ( w - ( DP_NUM_COLORS - 1 ) * gap ) / DP_NUM_COLORS;

		for ( int i = 0; i < DP_NUM_COLORS; i++ ) {
			const float sx = x + i * ( sw + gap );
			const float c[4] = { dpColors[i].r, dpColors[i].g, dpColors[i].b, 1.0f };

			DP_Fill( sx, y, sw, 13, c );
			if ( i == cl_datapadColor->integer )
				DP_Frame( sx - 1, y - 2, sw + 2, 17, dpWhite );
			else if ( DP_InRect( sx, y, sw, 13 ) )
				DP_Frame( sx, y - 1, sw, 15, dpText );
			if ( DP_Clicked( sx, y, sw, 13 ) )
				Cvar_SetValue( "cl_datapadColor", i );
		}
	}
	y += 22;

	// the keys that open and close it: click, then press one
	DP_Text( x, y, w, "Keys (Esc always puts it away)", dpDim, dp.font, small, qfalse );
	y += 12;
	DP_Text( x, y + 3, label, "Open it", dpText, dp.font, small, qfalse );
	if ( DP_Button( x + label, y, w - label, dp.binding == DP_BIND_OPEN ? "Press a key" : DP_KeyFor( DP_OPEN_CMD ), (qboolean)( dp.binding == DP_BIND_OPEN ) ) )
		dp.binding = DP_BIND_OPEN;
	y += DP_CTRL_H + 3;
	DP_Text( x, y + 3, label, "Close it", dpText, dp.font, small, qfalse );
	if ( DP_Button( x + label, y, w - label, dp.binding == DP_BIND_CLOSE ? "Press a key" : DP_KeyFor( DP_CLOSE_CMD ), (qboolean)( dp.binding == DP_BIND_CLOSE ) ) )
		dp.binding = DP_BIND_CLOSE;
	y += DP_CTRL_H + 9;

	// the character holding one while it's open
	DP_Text( x, y, w, "Your character", dpDim, dp.font, small, qfalse );
	y += 12;
	DP_Text( x, y + 3, label, "Holds it", dpText, dp.font, small, qfalse );
	if ( DP_Button( x + label, y, w - label, cl_datapadEmote->integer ? "On" : "Off", (qboolean)( cl_datapadEmote->integer != 0 ) ) ) {
		Cvar_Set( "cl_datapadEmote", cl_datapadEmote->integer ? "0" : "1" );
		// at once, as it's open now
		DP_Emote( (qboolean)( cl_datapadEmote->integer != 0 ) );
	}
	y += DP_CTRL_H + 2;
	DP_Text( x, y, w, "/" DP_EMOTE " while it's open", dpDim, dp.font, 0.55f, qfalse );
}

// the screen's apps: the home screen, and one sliding in over it from the right
static void DP_DrawApps( const dpRect_t *s ) {
	const float t = DP_Ease( cls.realtime - dp.appTime, DP_APP_MS );
	dpRect_t in, out;

	DP_StatusBar( s, 0.62f );
	// the apps in files of their own draw themselves, once the datapad has grown to them
	if ( dpAppFull[dp.app] || ( dpAppFull[dp.appFrom] && t < 1.0f ) ) {
		if ( t < 1.0f )
			dp.click = qfalse;
		else if ( dp.app == DP_PAZAAK )
			CL_Pazaak_Draw( s );
		else if ( dp.app == DP_HOLONET )
			CL_HoloNet_Draw( s );
		else if ( dp.app == DP_COMPORT )
			CL_Comport_Draw( s );
		return;
	}
	if ( t >= 1.0f || dp.app == dp.appFrom ) {
		if ( dp.app == DP_SETTINGS )
			DP_DrawSettings( s );
		else
			DP_DrawHome( s );
		return;
	}
	// on its way: drawn where it is, nothing clickable meanwhile
	dp.click = qfalse;
	in = out = *s;
	if ( dp.app == DP_SETTINGS ) {
		in.x = s->x + s->w * ( 1.0f - t );
		out.x = s->x - s->w * 0.3f * t;
		DP_DrawHome( &out );
		DP_Fill( in.x, s->y + DP_STATUS_H, s->x + s->w - in.x, s->h - DP_STATUS_H, dpScreen );
		DP_DrawSettings( &in );
	} else {
		out.x = s->x + s->w * t;
		in.x = s->x - s->w * 0.3f * ( 1.0f - t );
		DP_DrawHome( &in );
		DP_Fill( out.x, s->y + DP_STATUS_H, s->x + s->w - out.x, s->h - DP_STATUS_H, dpScreen );
		DP_DrawSettings( &out );
	}
}

void CL_Datapad_Draw( void ) {
	dpRect_t r, open, s;
	float big;

	dp.drawn = qfalse;
	if ( cls.state != CA_ACTIVE || !dp.font )
		return;
	// out of the way of a cinematic, folded
	if ( dp.state == DP_FOLDED && CL_RpCinematic_Active() )
		return;
	if ( !DP_Rect( &r ) )
		return;
	dp.drawn = qtrue;
	dp.last = r;
	DP_Colors();
	DP_FullRect( &open );
	big = Com_Clamp( 0.0f, 1.0f, ( r.h - DP_SMALL_H ) / ( open.h * 0.5f ) );
	DP_DrawCase( &r, big, &s );

	if ( dp.state == DP_OPEN && cls.realtime - dp.moveTime >= DP_SLIDE_MS ) {
		const float pad = DP_Wide( 6 ), gap = DP_Wide( 4 ), bw = ( r.w - pad * 2 - gap ) / 2.0f;
		const float by = Q_min( r.y + r.h, (float)SCREEN_HEIGHT ) - DP_CTRL_H - 5;

		DP_DrawApps( &s );
		if ( DP_Button( r.x + pad, by, bw, "Home", (qboolean)( dp.app == DP_HOME ) ) )
			DP_SetApp( DP_HOME );
		if ( DP_Button( r.x + pad + bw + gap, by, bw, "Close", qfalse ) ) {
			DP_Set( DP_CLOSED );
			return;
		}
		if ( dp.message[0] && cls.realtime - dp.messageTime < 3000 )
			DP_Text( s.x + 4, s.y + s.h - 14, s.w - 8, dp.message, dpText, dp.font, 0.62f, qfalse );
		dp.click = qfalse;
		re->SetColor( NULL );
		re->DrawStretchPic( dp.cursorX, dp.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
	} else if ( dp.state == DP_FOLDED && cls.realtime - dp.moveTime >= DP_SLIDE_MS ) {
		const int key = Key_GetKey( DP_OPEN_CMD ) >= 0 ? Key_GetKey( DP_OPEN_CMD ) : Key_GetKey( DP_CMD );

		DP_StatusBar( &s, 0.55f );
		DP_Text( s.x, s.y + s.h * 0.5f - 2, s.w, "No new entries", dpDim, dp.font, 0.62f, qtrue );
		DP_Text( r.x + 6, r.y + r.h - 10, r.w - 12, key >= 0 ? va( "%s: open", Key_KeynumToString( key ) ) : "/" DP_CMD ": open",
			dpDim, dp.font, 0.55f, qfalse );
	}
}

/*
===============================================================================

Keys and the mouse, while it's open

===============================================================================
*/

void CL_Datapad_KeyEvent( int key, qboolean down ) {
	const char *binding;

	if ( dp.state != DP_OPEN || !down )
		return;
	// a key picked in Settings: it alone does it from now on
	if ( dp.binding != DP_BIND_NONE && key != A_MOUSE1 && key != A_MOUSE2 ) {
		const qboolean opens = (qboolean)( dp.binding == DP_BIND_OPEN );
		const char *command = opens ? DP_OPEN_CMD : DP_CLOSE_CMD;

		dp.binding = DP_BIND_NONE;
		DP_Unbind( command );
		Key_SetBinding( key, command );
		DP_Message( va( "%s %s it", Key_KeynumToString( key ), opens ? "opens" : "closes" ) );
		return;
	}
	if ( key == A_MOUSE1 ) {
		dp.click = qtrue;
		dp.clickX = dp.cursorX;
		dp.clickY = dp.cursorY;
		return;
	}
	if ( ( dp.app == DP_PAZAAK && CL_Pazaak_Key( key ) ) || ( dp.app == DP_HOLONET && CL_HoloNet_Key( key ) )
		|| ( dp.app == DP_COMPORT && CL_Comport_Key( key ) ) )
		return;
	// its keys work while it's open, as they do in the game
	binding = Key_GetBinding( key );
	if ( VALIDSTRING( binding ) && !Q_stricmpn( binding, DP_CMD, strlen( DP_CMD ) ) )
		Cbuf_AddText( va( "%s\n", binding ) );
}

void CL_Datapad_CharEvent( int ch ) {
	if ( dp.state == DP_OPEN && dp.app == DP_PAZAAK )
		CL_Pazaak_Char( ch );
	else if ( dp.state == DP_OPEN && dp.app == DP_COMPORT )
		CL_Comport_Char( ch );
}

void CL_Datapad_MouseEvent( int dx, int dy ) {
	dp.cursorX = Com_Clamp( 0, SCREEN_WIDTH, dp.cursorX + dx );
	dp.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, dp.cursorY + dy );
}

// Esc: a key being picked is left as it was; otherwise it's put away
void CL_Datapad_Escape( void ) {
	if ( dp.binding != DP_BIND_NONE ) {
		dp.binding = DP_BIND_NONE;
		return;
	}
	DP_Set( DP_CLOSED );
}

/*
===============================================================================

The command

===============================================================================
*/

static void DP_Datapad_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		DP_Set( dp.state == DP_OPEN ? DP_CLOSED : DP_OPEN );
	} else if ( !Q_stricmp( arg, "open" ) ) {
		DP_Set( DP_OPEN );
	} else if ( !Q_stricmp( arg, "settings" ) || !Q_stricmp( arg, "pazaak" ) || !Q_stricmp( arg, "holonet" ) || !Q_stricmp( arg, "comport" ) ) {
		DP_Set( DP_OPEN );
		if ( dp.state == DP_OPEN )
			DP_SetApp( !Q_stricmp( arg, "pazaak" ) ? DP_PAZAAK : !Q_stricmp( arg, "holonet" ) ? DP_HOLONET
				: !Q_stricmp( arg, "comport" ) ? DP_COMPORT : DP_SETTINGS );
		// straight to an article, or a forum
		if ( dp.state == DP_OPEN && Cmd_Argc() > 2 ) {
			if ( !Q_stricmp( arg, "holonet" ) )
				CL_HoloNet_Open( atoi( Cmd_Argv( 2 ) ) );
			else if ( !Q_stricmp( arg, "comport" ) )
				CL_Comport_Open( Cmd_Argv( 2 ) );
		}
	} else if ( !Q_stricmp( arg, "fold" ) ) {
		DP_Set( DP_FOLDED );
	} else if ( !Q_stricmp( arg, "close" ) ) {
		DP_Set( DP_CLOSED );
	} else if ( !Q_stricmp( arg, "pos" ) ) {
		if ( Cmd_Argc() < 4 ) {
			Cvar_Set( "cl_datapadX", "-1" );
			Cvar_Set( "cl_datapadY", "-1" );
			Com_Printf( "Datapad: back in its place above the HUD\n" );
		} else {
			Cvar_Set( "cl_datapadX", Cmd_Argv( 2 ) );
			Cvar_Set( "cl_datapadY", Cmd_Argv( 3 ) );
		}
	} else {
		Com_Printf( "usage: " DP_CMD "                opens the datapad, or puts it away\n" );
		Com_Printf( "       " DP_CMD " open|close\n" );
		Com_Printf( "       " DP_CMD " settings       open on its Settings\n" );
		Com_Printf( "       " DP_CMD " pazaak|holonet|comport   open on that app\n" );
		Com_Printf( "       " DP_CMD " pos <x> <y>    where it opens, on the 640x480 screen (its top left)\n" );
		Com_Printf( "       " DP_CMD " pos            back in the bottom right corner, above the HUD\n" );
	}
}

void CL_Datapad_Init( void ) {
	memset( &dp, 0, sizeof( dp ) );
	cl_datapadX = Cvar_Get( "cl_datapadX", "-1", CVAR_ARCHIVE_ND, "Where the folded datapad goes, from the left of the 640x480 screen; -1: the bottom right corner, above the HUD" );
	cl_datapadY = Cvar_Get( "cl_datapadY", "-1", CVAR_ARCHIVE_ND, "Where the folded datapad goes, from the top of the 640x480 screen; -1: the bottom right corner, above the HUD" );
	cl_datapadColor = Cvar_Get( "cl_datapadColor", "0", CVAR_ARCHIVE_ND, "The datapad's colour: 0 cyan, 1 blue, 2 green, 3 amber, 4 red, 5 violet, 6 white" );
	cl_datapadEmote = Cvar_Get( "cl_datapadEmote", "0", CVAR_ARCHIVE_ND, "Your character holds a datapad (RPMod's /emlift) while yours is open" );
	Cmd_AddCommand( DP_CMD, DP_Datapad_f, "The datapad: open like a phone, folded in the corner, or put away" );
	CL_Pazaak_Init();
	CL_Comport_Init();
}
