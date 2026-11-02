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

// cl_rpcinematic.cpp -- a cinematic camera, driven by hooks in RPMod's prints
//
// An admin's rpnotify -1 1 cmd:cin/<verb>/<parts...> takes every player with this
// client from their own view to a camera the story moves around (see cl_rphooks.cpp
// for how the hooks travel). A spot is "x y z [pitch yaw [roll [fov]]]"; a fov is
// cg_fov's, horizontal on a 4:3 screen, so every screen frames the same.
//
//   cmd:cin/start[/hud][/bars]      the camera takes over where the player looks from;
//                                   the HUD is hidden unless hud, bars slide in with bars
//   cmd:cin/path[/linear][/look <x y z>]/<spot>[/<seconds>/<spot>...]
//                                   a shot: the camera cuts to the first spot, then glides
//                                   through the others, each the seconds before it after
//                                   the one before; look keeps it turned to the point
//   cmd:cin/move/<seconds>[/linear]/<spot>[/<seconds>/<spot>...]
//                                   the same from where the camera is; spots without their
//                                   own seconds share the first out
//                                   Before the spots, a path or orbit can also have
//                                   dof <focus> <range> <blur> [<who>]: what's focus away is
//                                   sharp, give or take range, the rest blurred up to blur
//                                   (1-10); focus 0 is whatever the camera looks at, -1 who
//                                   (a player or NPC, by number; a name where it's played)
//                                   filter <name> <strength %>: one of rcFilters' words over
//                                   the picture
//                                   A path's spot can end in its own <focus> <range> <blur>
//                                   after its fov: the depth of field goes from one spot's
//                                   to the next's
//                                   in|out <cut|fade|left|right|top|bottom> <seconds> <r g b>:
//                                   the shot fades in from the colour, or is wiped in from
//                                   that side, and out to it at its end; the screen stays
//                                   that colour after an out until the next shot
//   cmd:cin/cut/<spot>              the camera jumps there
//   cmd:cin/orbit/<seconds>[/linear]/<x y z>/<radius>/<height>/<from yaw>/<degrees>[/<fov>[/<end fov>]]
//                                   circles the point looking at it, from the yaw it is
//                                   seen from, round by the degrees (negative goes the
//                                   other way, 720 is twice round); one fov zooms to it
//                                   from where it is, two go from the first to the second
//   cmd:cin/lookat/<x y z>|off      keeps the camera turned to the point
//   cmd:cin/fov/<fov>[/<seconds>]   zooms
//   cmd:cin/bars/on|off[/<seconds>] the black bars, top and bottom
//   cmd:cin/hud/on|off              the HUD; full screen fades and overlays always show
//   cmd:cin/end                     back to the player's own view
//
// Camera commands start a cinematic that hasn't been. rpc_cinematics 0 leaves them all
// out, and /cincam stop leaves the one playing. The server still only sends what is
// near each player, so things far from them may be missing in a shot of somewhere else.
//
// Cinematics are made in the cinematic manager (cl_cinematic.cpp), which plays them on
// every screen itself through the camera here. Before it, single shots were made in a camera
// editor and saved in camshots/<name>.cam as their path or orbit hook, which /cincam play
// and the Storyteller's old Camera shot steps still send. NPCs walking in the shot follow it as
//   walk <npc> <route> walk|run <seconds into the shot>
// lines, sent by whoever plays the shot (npcmanager walk, see cl_npcwalk.cpp) and walked
// on the editor's screen in its preview. Players and NPCs playing one of JKA's animations
// in it, a range of its frames, backwards, or one frame held, follow as
//   anim <who> <ANIMATION> <from> <to> play|reverse|hold loop|once <speed %> <seconds in> <seconds>
// lines, sent in a cmd:anim hook (see Animations below).

#include "client.h"
#include "qcommon/cm_public.h"
#include "ghoul2/G2.h"
#include "cl_rpcinematic.h"

#define RC_BAR_MS			1000

#define RC_SHOT_HEADER		"// camera shot, made with /cincam edit\n"
#define RC_MAX_QUEUED		16

// walks of a shot played, each run when its time comes and, sent, when the server takes it
static struct {
	char		cmd[MAX_STRING_CHARS];
	int			at;					// cls.realtime
	qboolean	local;
} rcQueue[RC_MAX_QUEUED];
static int rcNumQueued;

static const char *rcTransWords[RC_NUM_TRANSITIONS] = { "cut", "fade", "left", "right", "top", "bottom" };

#define RC_DOF_FOCUS_RATE	6.0f		// an automatic focus follows what's looked at this quickly
#define RC_DOF_FOLLOW_RATE	12.0f		// and someone followed this quickly
#define RC_DOF_FOLLOW_UP	24.0f		// someone followed is in focus this far over their middle
#define RC_WIPE_SOFT		0.06f		// a wipe's edge, soft over this much of the screen

typedef struct {
	qboolean	active;
	qboolean	showHud;

	// where the camera is, worked out once a frame
	vec3_t		org, ang;
	float		fovY;
	int			frame;

	// a shot playing, or where it holds
	qboolean	playing;
	rcShot_t	shot;
	int			start;
	qboolean	shotFov;		// the shot zooms, until a fov hook says otherwise
	vec3_t		holdOrg, holdAng;

	// the lookat hook
	qboolean	lookAt;
	vec3_t		lookTarget;

	float		fovFrom, fovTo;
	int			fovStart, fovDuration;

	// the last shot's transitions, its out holding after it; qfalse after a cut
	qboolean	trans;

	// the last shot's depth of field and filter, until another shot; dofNow is where a focus
	// that follows something has got to
	qboolean	dof;
	qboolean	filter;
	float		dofNow;
	int			dofTime;

	// the bars, on their own as well
	qboolean	bars;
	float		barsFrom;
	int			barsStart, barsDuration;

	// the player's own view, the last the cgame drew
	qboolean	haveView;
	vec3_t		viewOrg, viewAng;
	float		viewFovY;
} rcState_t;

static rcState_t rc;

static cvar_t *rpc_cinematics;
static cvar_t *rpc_cinThirdPerson;		// 1 while cg_thirdPerson is ours to put back to 0, a crash included

static int RC_FilterByWord( const char *word );
static const char *RC_FilterWord( int type );
static void RC_StopAnims( qboolean editorToo );

// the cgame only draws the player's own body in third person; the camera is ours either way
static void RC_ThirdPerson( qboolean on ) {
	if ( !rpc_cinThirdPerson )
		return;		// a disconnect before the client is up
	if ( on ) {
		if ( Cvar_VariableIntegerValue( "cg_thirdPerson" ) || rpc_cinThirdPerson->integer )
			return;
		Cvar_Set( "rpc_cinThirdPerson", "1" );
		Cvar_Set( "cg_thirdPerson", "1" );
	} else if ( rpc_cinThirdPerson->integer ) {
		Cvar_Set( "cg_thirdPerson", "0" );
		Cvar_Set( "rpc_cinThirdPerson", "0" );
	}
}

void RC_SyncThirdPerson( void ) {
	RC_ThirdPerson( (qboolean)( rc.active || CM_EditorOpen() ) );
}

float RC_Ease( float t, qboolean linear ) {
	t = Com_Clamp( 0.0f, 1.0f, t );
	return linear ? t : t * t * ( 3.0f - 2.0f * t );
}

// a fov as cg_fov gives it, horizontal on 4:3, as a vertical one any screen can use
float RC_FovY( float fovX ) {
	fovX = Com_Clamp( 1.0f, 170.0f, fovX );
	return RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovX * 0.5f ) ) * 0.75f ) );
}

float RC_FovX( float fovY, float width, float height ) {
	return RAD2DEG( 2.0f * atanf( tanf( DEG2RAD( fovY * 0.5f ) ) * width / height ) );
}

int RC_Round( float v ) {
	return (int)floorf( v + 0.5f );
}

/*
===============================================================================

Shots

===============================================================================
*/

static float RC_CatmullRom( float p0, float p1, float p2, float p3, float t ) {
	return 0.5f * ( 2.0f * p1 + ( p2 - p0 ) * t + ( 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 ) * t * t
		+ ( 3.0f * p1 - p0 - 3.0f * p2 + p3 ) * t * t * t );
}

int RC_ShotMs( const rcShot_t *s ) {
	int ms = 0;

	if ( s->orbit || s->numKeys == 1 )
		return s->ms;
	for ( int i = 1; i < s->numKeys; i++ )
		ms += s->keys[i].ms;
	return ms;
}

// where the shot's camera is, ms into it; the easing is over the whole shot
void RC_EvalShot( const rcShot_t *s, int ms, vec3_t org, vec3_t ang, float *fov ) {
	const int total = RC_ShotMs( s );
	const float t = total > 0 ? RC_Ease( (float)ms / total, s->linear ) : 1.0f;

	if ( s->orbit ) {
		const float yaw = DEG2RAD( s->fromYaw + s->degrees * t );
		vec3_t dir;

		org[0] = s->center[0] + cosf( yaw ) * s->radius;
		org[1] = s->center[1] + sinf( yaw ) * s->radius;
		org[2] = s->center[2] + s->height;
		VectorSubtract( s->center, org, dir );
		vectoangles( dir, ang );
		ang[ROLL] = 0;
		*fov = s->fov + ( s->endFov - s->fov ) * t;
		return;
	}

	if ( !s->numKeys ) {
		VectorClear( org );
		VectorClear( ang );
		*fov = RC_DEFAULT_FOV;
		return;
	}
	if ( s->numKeys == 1 || total <= 0 ) {
		const rcKey_t *k = &s->keys[s->numKeys - 1];

		VectorCopy( k->org, org );
		VectorCopy( k->ang, ang );
		*fov = k->fov;
	} else {
		// a smooth curve through the keys, each stretch as long as its key says
		const float u = t * total;
		float from = 0;
		int i;

		for ( i = 1; i < s->numKeys - 1 && u > from + s->keys[i].ms; i++ )
			from += s->keys[i].ms;
		const float local = s->keys[i].ms > 0 ? Com_Clamp( 0.0f, 1.0f, ( u - from ) / s->keys[i].ms ) : 1.0f;
		const rcKey_t *k0 = &s->keys[Q_max( i - 2, 0 )], *k1 = &s->keys[i - 1], *k2 = &s->keys[i];
		const rcKey_t *k3 = &s->keys[Q_min( i + 1, s->numKeys - 1 )];

		for ( int j = 0; j < 3; j++ ) {
			org[j] = RC_CatmullRom( k0->org[j], k1->org[j], k2->org[j], k3->org[j], local );
			ang[j] = RC_CatmullRom( k0->ang[j], k1->ang[j], k2->ang[j], k3->ang[j], local );
		}
		*fov = RC_CatmullRom( k0->fov, k1->fov, k2->fov, k3->fov, local );
	}

	if ( s->look ) {
		const float roll = ang[ROLL];
		vec3_t dir;

		VectorSubtract( s->lookAt, org, dir );
		if ( VectorLength( dir ) > 1.0f ) {
			vectoangles( dir, ang );
			ang[ROLL] = roll;
		}
	}
}

// fovs not given are the one before's, the first's the camera's
void RC_FillFov( rcShot_t *s, float fov ) {
	if ( s->orbit ) {
		if ( s->fov <= 0 )
			s->fov = fov;
		if ( s->endFov <= 0 )
			s->endFov = s->fov;
		return;
	}
	for ( int i = 0; i < s->numKeys; i++ ) {
		if ( s->keys[i].fov <= 0 )
			s->keys[i].fov = i ? s->keys[i - 1].fov : fov;
	}
}

int RC_SecondsMs( float seconds ) {
	return Q_max( 0, (int)( seconds * 1000.0f + 0.5f ) );
}

static int RC_Ms( const char *seconds ) {
	return RC_SecondsMs( (float)atof( seconds ) );
}

static qboolean RC_IsWord( const char *s ) {
	return (qboolean)( isalpha( (unsigned char)s[0] ) != 0 );
}

// a shot's option word: linear, look, dof, in, out; qtrue if it was one of them
static qboolean RC_ParseOption( const char *p, rcShot_t *s ) {
	if ( !Q_stricmp( p, "linear" ) ) {
		s->linear = qtrue;
		return qtrue;
	}
	if ( !Q_stricmpn( p, "look ", 5 ) ) {
		if ( sscanf( p + 5, "%f %f %f", &s->lookAt[0], &s->lookAt[1], &s->lookAt[2] ) == 3 )
			s->look = qtrue;
		return qtrue;
	}
	if ( !Q_stricmpn( p, "dofpt ", 6 ) ) {
		float sec, v[4];

		if ( sscanf( p + 6, "%f %f %f %f %f", &sec, &v[0], &v[1], &v[2], &v[3] ) == 5 && s->numFocus < RC_MAX_FOCUS ) {
			rcFocus_t f;
			int i = s->numFocus++;

			f.ms = RC_SecondsMs( sec );
			VectorCopy( v, f.org );
			f.range = Q_max( 8.0f, v[3] );
			// in order of when
			for ( ; i > 0 && s->focus[i - 1].ms > f.ms; i-- )
				s->focus[i] = s->focus[i - 1];
			s->focus[i] = f;
		}
		return qtrue;
	}
	if ( !Q_stricmpn( p, "dof ", 4 ) ) {
		float v[3];
		int n = 0;

		if ( sscanf( p + 4, "%f %f %f %n", &v[0], &v[1], &v[2], &n ) >= 3 && v[2] > 0.0f ) {
			s->dof = qtrue;
			s->dofFocus = v[0] <= -1.5f ? RC_DOF_POINT : v[0] < 0.0f ? -1.0f : v[0];
			s->dofRange = Q_max( 1.0f, v[1] );
			s->dofBlur = Com_Clamp( 0.0f, RC_DOF_MAX_BLUR, v[2] );
			// who it follows, after the numbers
			if ( n > 0 )
				Q_strncpyz( s->dofWho, p + 4 + n, sizeof( s->dofWho ) );
			if ( s->dofFocus == -1.0f && !s->dofWho[0] )
				s->dofFocus = 0.0f;
		}
		return qtrue;
	}
	if ( !Q_stricmpn( p, "filter ", 7 ) ) {
		char word[32];
		int strength = 100;

		if ( sscanf( p + 7, "%31s %i", word, &strength ) >= 1 ) {
			s->filter = RC_FilterByWord( word );
			s->filterStrength = Com_Clampi( 0, 100, strength );
			if ( !s->filterStrength )
				s->filter = 0;
		}
		return qtrue;
	}
	if ( !Q_stricmpn( p, "in ", 3 ) || !Q_stricmpn( p, "out ", 4 ) ) {
		rcTrans_t *tr = !Q_stricmpn( p, "in ", 3 ) ? &s->in : &s->out;
		char word[16];
		float seconds = 0;
		int r = 0, g = 0, b = 0;

		if ( sscanf( strchr( p, ' ' ) + 1, "%15s %f %i %i %i", word, &seconds, &r, &g, &b ) >= 2 ) {
			for ( int i = 0; i < RC_NUM_TRANSITIONS; i++ ) {
				if ( !Q_stricmp( word, rcTransWords[i] ) )
					tr->type = i;
			}
			tr->ms = RC_SecondsMs( seconds );
			tr->color[0] = (byte)Com_Clampi( 0, 255, r );
			tr->color[1] = (byte)Com_Clampi( 0, 255, g );
			tr->color[2] = (byte)Com_Clampi( 0, 255, b );
			if ( !tr->ms )
				tr->type = RC_TR_CUT;
		}
		return qtrue;
	}
	return qfalse;
}

// the shot, or one of its spots, keeps someone in focus
static qboolean RC_ShotFollows( const rcShot_t *s ) {
	if ( !s->dof )
		return qfalse;
	if ( s->dofFocus < 0.0f )
		return qtrue;
	for ( int i = 0; i < s->numKeys; i++ ) {
		if ( s->keys[i].dofSet && s->keys[i].dofFocus < 0.0f )
			return qtrue;
	}
	return qfalse;
}

// the options as a hook writes them, each after a /
static void RC_WriteOptions( const rcShot_t *s, char *out, int size ) {
	const rcTrans_t *trs[2] = { &s->in, &s->out };

	if ( s->dof ) {
		Q_strcat( out, size, va( "/dof %i %i %g", RC_Round( s->dofFocus ), RC_Round( s->dofRange ), s->dofBlur ) );
		if ( s->dofWho[0] && ( s->dofFocus < 0.0f || RC_ShotFollows( s ) ) )
			Q_strcat( out, size, va( " %s", s->dofWho ) );
		for ( int i = 0; s->dofFocus == RC_DOF_POINT && i < s->numFocus; i++ ) {
			const rcFocus_t *f = &s->focus[i];

			Q_strcat( out, size, va( "/dofpt %g %i %i %i %i", RC_Seconds( f->ms ), RC_Round( f->org[0] ), RC_Round( f->org[1] ), RC_Round( f->org[2] ),
				RC_Round( f->range ) ) );
		}
	}
	if ( s->filter && s->filterStrength > 0 )
		Q_strcat( out, size, va( "/filter %s %i", RC_FilterWord( s->filter ), s->filterStrength ) );
	for ( int i = 0; i < 2; i++ ) {
		const rcTrans_t *tr = trs[i];

		if ( tr->type != RC_TR_CUT && tr->ms > 0 )
			Q_strcat( out, size, va( "/%s %s %g %i %i %i", i ? "out" : "in", rcTransWords[tr->type], floorf( tr->ms / 100.0f + 0.5f ) / 10.0f,
				tr->color[0], tr->color[1], tr->color[2] ) );
	}
}

// angles turned the short way round from the spot before, so a curve through them doesn't spin
static void RC_Unwrap( rcKey_t *key, const rcKey_t *prev ) {
	for ( int j = 0; j < 3; j++ )
		key->ang[j] = prev->ang[j] + AngleNormalize180( key->ang[j] - prev->ang[j] );
}

// the parts of a path or a move, onto the keys there are: [<seconds>] [linear] [look <x y z>],
// then the spots, each after its own seconds if it has them; spots without share the first
// seconds out. A spot without angles keeps the one before's, the first's ang
static void RC_ParseKeys( char **argv, int argc, rcShot_t *s, const vec3_t ang ) {
	const int first = s->numKeys;
	float lead = -1, own = -1;
	int without = 0;

	for ( int i = 0; i < argc; i++ ) {
		const char *p = argv[i];
		float v[10];
		int n;

		if ( RC_IsWord( p ) ) {
			RC_ParseOption( p, s );
			continue;
		}
		n = sscanf( p, "%f %f %f %f %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9] );
		if ( n == 1 ) {
			if ( s->numKeys == first && lead < 0 )
				lead = v[0];
			else
				own = v[0];
			continue;
		}
		if ( n < 3 || s->numKeys > RC_MAX_KEYS )
			continue;

		rcKey_t *key = &s->keys[s->numKeys];
		const rcKey_t *prev = s->numKeys ? &s->keys[s->numKeys - 1] : NULL;

		VectorSet( key->org, v[0], v[1], v[2] );
		VectorCopy( prev ? prev->ang : ang, key->ang );
		if ( n >= 5 ) {
			key->ang[PITCH] = v[3];
			key->ang[YAW] = v[4];
			key->ang[ROLL] = n >= 6 ? v[5] : 0;
		}
		key->fov = n >= 7 ? v[6] : 0;
		key->dofSet = (qboolean)( n >= 10 );
		if ( key->dofSet ) {
			key->dofFocus = v[7] < 0.0f ? -1.0f : v[7];
			key->dofRange = Q_max( 1.0f, v[8] );
			key->dofBlur = Com_Clamp( 0.0f, RC_DOF_MAX_BLUR, v[9] );
		}
		key->ms = own >= 0 ? RC_SecondsMs( own ) : -1;
		if ( key->ms < 0 && s->numKeys )
			without++;
		own = -1;
		if ( prev )
			RC_Unwrap( key, prev );
		s->numKeys++;
	}
	if ( s->numKeys )
		s->keys[0].ms = 0;
	for ( int i = 1; i < s->numKeys; i++ ) {
		if ( s->keys[i].ms < 0 )
			s->keys[i].ms = without && lead > 0 ? RC_SecondsMs( lead / without ) : 0;
	}
}

// <seconds>[/linear]/<x y z>/<radius>/<height>/<from yaw>/<degrees>[/<fov>[/<end fov>]]
static qboolean RC_ParseOrbit( char **argv, int argc, rcShot_t *s ) {
	int part = 1;

	if ( argc < 1 )
		return qfalse;
	s->orbit = qtrue;
	s->ms = RC_Ms( argv[0] );
	while ( part < argc && RC_IsWord( argv[part] ) ) {
		RC_ParseOption( argv[part], s );
		part++;
	}
	if ( argc - part < 5 || sscanf( argv[part], "%f %f %f", &s->center[0], &s->center[1], &s->center[2] ) != 3 )
		return qfalse;
	s->radius = (float)atof( argv[part + 1] );
	s->height = (float)atof( argv[part + 2] );
	s->fromYaw = (float)atof( argv[part + 3] );
	s->degrees = (float)atof( argv[part + 4] );
	if ( argc - part > 6 && argv[part + 6][0] ) {
		s->fov = (float)atof( argv[part + 5] );
		s->endFov = (float)atof( argv[part + 6] );
	} else if ( argc - part > 5 && argv[part + 5][0] ) {
		s->endFov = (float)atof( argv[part + 5] );	// from where the camera is
	}
	return qtrue;
}

// a path or an orbit hook's parts after cin, as a shot; fovs not given are left 0
qboolean RC_ParseShot( char **argv, int argc, rcShot_t *s, const vec3_t ang ) {
	memset( s, 0, sizeof( *s ) );
	if ( argc < 2 )
		return qfalse;
	if ( !Q_stricmp( argv[0], "orbit" ) )
		return RC_ParseOrbit( argv + 1, argc - 1, s );
	if ( Q_stricmp( argv[0], "path" ) )
		return qfalse;
	RC_ParseKeys( argv + 1, argc - 1, s, ang );
	return (qboolean)( s->numKeys > 0 );
}

float RC_Seconds( int ms ) {
	return floorf( ms / 100.0f + 0.5f ) / 10.0f;
}

// the hook that plays the shot, rounded as it goes out
void RC_WriteShot( const rcShot_t *s, char *out, int size ) {
	if ( s->orbit ) {
		Com_sprintf( out, size, "cin/orbit/%g%s", RC_Seconds( s->ms ), s->linear ? "/linear" : "" );
		RC_WriteOptions( s, out, size );
		Q_strcat( out, size, va( "/%i %i %i/%i/%i/%i/%i/%i/%i",
			RC_Round( s->center[0] ), RC_Round( s->center[1] ), RC_Round( s->center[2] ), RC_Round( s->radius ), RC_Round( s->height ),
			RC_Round( AngleNormalize360( s->fromYaw ) ), RC_Round( s->degrees ), RC_Round( s->fov ), RC_Round( s->endFov ) ) );
		return;
	}
	Com_sprintf( out, size, "cin/path%s", s->linear ? "/linear" : "" );
	RC_WriteOptions( s, out, size );
	if ( s->look )
		Q_strcat( out, size, va( "/look %i %i %i", RC_Round( s->lookAt[0] ), RC_Round( s->lookAt[1] ), RC_Round( s->lookAt[2] ) ) );
	for ( int i = 0; i < s->numKeys; i++ ) {
		const rcKey_t *k = &s->keys[i];

		if ( i )
			Q_strcat( out, size, va( "/%g", RC_Seconds( k->ms ) ) );
		Q_strcat( out, size, va( "/%i %i %i %i %i %i %i", RC_Round( k->org[0] ), RC_Round( k->org[1] ), RC_Round( k->org[2] ),
			RC_Round( AngleNormalize180( k->ang[PITCH] ) ), RC_Round( AngleNormalize360( k->ang[YAW] ) ),
			RC_Round( AngleNormalize180( k->ang[ROLL] ) ), RC_Round( k->fov ) ) );
		if ( k->dofSet && s->dof )
			Q_strcat( out, size, va( " %i %i %g", RC_Round( k->dofFocus ), RC_Round( k->dofRange ), k->dofBlur ) );
	}
}

// a hook cut into its parts at the /, spaces round them trimmed, as cl_rphooks hands them over
int RC_SplitHook( char *hook, char **argv, int max ) {
	char *p = hook;
	int argc = 0;

	while ( p && argc < max ) {
		char *end = strchr( p, '/' );

		if ( end )
			*end = '\0';
		while ( *p == ' ' )
			p++;
		for ( char *t = p + strlen( p ); t > p && ( t[-1] == ' ' || t[-1] == '\r' || t[-1] == '\n' ); )
			*--t = '\0';
		argv[argc++] = p;
		p = end ? end + 1 : NULL;
	}
	return argc;
}

/*
===============================================================================

The camera

===============================================================================
*/

static float RC_Fov( void ) {
	if ( !rc.fovDuration || cls.realtime - rc.fovStart >= rc.fovDuration )
		return rc.fovTo;
	return rc.fovFrom + ( rc.fovTo - rc.fovFrom ) * RC_Ease( (float)( cls.realtime - rc.fovStart ) / rc.fovDuration, qfalse );
}

static void RC_Update( void ) {
	if ( rc.frame == cls.framecount )
		return;
	rc.frame = cls.framecount;

	if ( rc.playing ) {
		float fov;

		RC_EvalShot( &rc.shot, cls.realtime - rc.start, rc.org, rc.ang, &fov );
		rc.fovY = rc.shotFov ? RC_FovY( fov ) : RC_Fov();
	} else {
		VectorCopy( rc.holdOrg, rc.org );
		VectorCopy( rc.holdAng, rc.ang );
		rc.fovY = RC_Fov();
	}

	if ( rc.lookAt && !( rc.playing && ( rc.shot.orbit || rc.shot.look ) ) ) {
		const float roll = rc.ang[ROLL];
		vec3_t dir;

		VectorSubtract( rc.lookTarget, rc.org, dir );
		if ( VectorLength( dir ) > 1.0f ) {
			vectoangles( dir, rc.ang );
			rc.ang[ROLL] = roll;
		}
	}
}

// the fov a shot starts from, as cg_fov gives it
static float RC_CurrentFov( void ) {
	return RC_FovX( rc.fovY, 4.0f, 3.0f );
}

// the camera holds still where it is now
static void RC_Hold( void ) {
	RC_Update();
	if ( rc.playing && rc.shotFov ) {
		rc.fovTo = rc.fovY;
		rc.fovDuration = 0;
	}
	rc.playing = qfalse;
	rc.shotFov = qfalse;
	VectorCopy( rc.org, rc.holdOrg );
	VectorCopy( rc.ang, rc.holdAng );
	rc.frame = -1;
}

static void RC_Play( const rcShot_t *shot ) {
	rc.shot = *shot;
	rc.playing = qtrue;
	rc.shotFov = qtrue;
	rc.start = cls.realtime;
	rc.frame = -1;
	// its transitions, and its focus and filter until another shot; a focus that follows
	// something starts where the last left off, a set one where it's set
	rc.trans = qtrue;
	if ( !rc.dof )
		rc.dofNow = 0.0f;
	rc.dof = shot->dof;
	rc.filter = (qboolean)( shot->filter != 0 );
}

static void RC_ZoomTo( float fovY, int ms ) {
	rc.fovFrom = RC_Fov();
	rc.fovTo = fovY;
	rc.fovStart = cls.realtime;
	rc.fovDuration = ms;
}

static float RC_BarsNow( void ) {
	const float to = rc.bars ? 1.0f : 0.0f;

	if ( !rc.barsDuration || cls.realtime - rc.barsStart >= rc.barsDuration )
		return to;
	return rc.barsFrom + ( to - rc.barsFrom ) * RC_Ease( (float)( cls.realtime - rc.barsStart ) / rc.barsDuration, qfalse );
}

// the bars slide from wherever they are, halfway there too
static void RC_Bars( qboolean on, int ms ) {
	rc.barsFrom = RC_BarsNow();
	rc.bars = on;
	rc.barsStart = cls.realtime;
	rc.barsDuration = ms;
}

static void RC_Start( void ) {
	if ( rc.active )
		return;
	rc.active = qtrue;
	rc.showHud = qfalse;
	rc.lookAt = qfalse;
	rc.playing = qfalse;
	rc.shotFov = qfalse;
	rc.trans = qfalse;
	rc.dof = qfalse;
	rc.filter = qfalse;
	rc.frame = -1;
	if ( rc.haveView ) {
		VectorCopy( rc.viewOrg, rc.holdOrg );
		VectorCopy( rc.viewAng, rc.holdAng );
		rc.fovTo = rc.viewFovY;
	} else {
		VectorCopy( cl.snap.ps.origin, rc.holdOrg );
		rc.holdOrg[2] += cl.snap.ps.viewheight;
		VectorCopy( cl.viewangles, rc.holdAng );
		rc.fovTo = RC_FovY( RC_DEFAULT_FOV );
	}
	rc.fovDuration = 0;
	RC_SyncThirdPerson();
}

// a cinematic starts: the camera takes over, the HUD and the bars as it has them
void RC_CineBegin( qboolean hud, qboolean bars ) {
	RC_Start();
	rc.showHud = hud;
	rc.lookAt = qfalse;
	if ( bars != rc.bars )
		RC_Bars( bars, RC_BAR_MS );
}

// one of its shots, msIn ms into it already
void RC_CineShot( const rcShot_t *shot, int msIn ) {
	RC_Start();
	RC_Hold();
	rc.lookAt = qfalse;
	RC_Play( shot );
	rc.start -= msIn;
	rc.frame = -1;
}

// where the player looked from, the last the cgame drew; qfalse before it has
qboolean RC_PlayerView( vec3_t org, vec3_t ang, float *fovY ) {
	if ( !rc.haveView )
		return qfalse;
	VectorCopy( rc.viewOrg, org );
	VectorCopy( rc.viewAng, ang );
	*fovY = rc.viewFovY;
	return qtrue;
}

void CL_RpCinematic_Stop( void ) {
	// the shot's animations go with it
	RC_StopAnims( qfalse );
	if ( rc.bars )
		RC_Bars( qfalse, RC_BAR_MS / 2 );
	rc.active = qfalse;
	rc.playing = qfalse;
	rc.trans = qfalse;
	rc.dof = qfalse;
	rc.filter = qfalse;
	RC_SyncThirdPerson();
}

/*
===============================================================================

Hooks

===============================================================================
*/

// cmd:cin/<verb>/<parts...>, the parts after cin; the settings aren't asked
static void RC_Hook( char **argv, int argc ) {
	const char *verb = argc ? argv[0] : "";
	char **parts = argv + 1;
	const int numParts = argc - 1;
	rcShot_t shot;

	if ( !Q_stricmp( verb, "start" ) ) {
		qboolean bars = qfalse;

		RC_Start();
		for ( int i = 0; i < numParts; i++ ) {
			if ( !Q_stricmp( parts[i], "hud" ) )
				rc.showHud = qtrue;
			else if ( !Q_stricmp( parts[i], "bars" ) )
				bars = qtrue;
		}
		if ( bars )
			RC_Bars( qtrue, RC_BAR_MS );
	} else if ( !Q_stricmp( verb, "end" ) ) {
		CL_RpCinematic_Stop();
	} else if ( !Q_stricmp( verb, "cut" ) ) {
		if ( !numParts )
			return;
		RC_Start();
		RC_Hold();
		memset( &shot, 0, sizeof( shot ) );
		RC_ParseKeys( parts, 1, &shot, rc.ang );
		if ( !shot.numKeys )
			return;
		VectorCopy( shot.keys[0].org, rc.holdOrg );
		VectorCopy( shot.keys[0].ang, rc.holdAng );
		rc.trans = qfalse;
		if ( shot.keys[0].fov > 0 )
			RC_ZoomTo( RC_FovY( shot.keys[0].fov ), 0 );
		rc.frame = -1;
	} else if ( !Q_stricmp( verb, "move" ) ) {
		if ( numParts < 2 )
			return;
		RC_Start();
		RC_Hold();
		memset( &shot, 0, sizeof( shot ) );
		VectorCopy( rc.org, shot.keys[0].org );
		VectorCopy( rc.ang, shot.keys[0].ang );
		shot.keys[0].fov = RC_CurrentFov();
		shot.numKeys = 1;
		RC_ParseKeys( parts, numParts, &shot, rc.ang );
		if ( shot.numKeys < 2 )
			return;
		RC_FillFov( &shot, shot.keys[0].fov );
		RC_Play( &shot );
	} else if ( !Q_stricmp( verb, "path" ) || !Q_stricmp( verb, "orbit" ) ) {
		RC_Start();
		RC_Hold();
		if ( !RC_ParseShot( argv, argc, &shot, rc.ang ) )
			return;
		RC_FillFov( &shot, RC_CurrentFov() );
		// a path is a shot of its own, it looks where it says
		if ( !shot.orbit )
			rc.lookAt = qfalse;
		RC_Play( &shot );
	} else if ( !Q_stricmp( verb, "lookat" ) ) {
		if ( !numParts || !Q_stricmp( parts[0], "off" ) ) {
			RC_Hold();
			rc.lookAt = qfalse;
		} else if ( sscanf( parts[0], "%f %f %f", &rc.lookTarget[0], &rc.lookTarget[1], &rc.lookTarget[2] ) == 3 ) {
			RC_Start();
			rc.lookAt = qtrue;
			rc.frame = -1;
		}
	} else if ( !Q_stricmp( verb, "fov" ) ) {
		if ( !numParts )
			return;
		RC_Start();
		RC_Update();
		if ( rc.shotFov ) {
			rc.fovTo = rc.fovY;
			rc.fovDuration = 0;
			rc.shotFov = qfalse;
		}
		RC_ZoomTo( RC_FovY( (float)atof( parts[0] ) ), numParts > 1 ? RC_Ms( parts[1] ) : 0 );
		rc.frame = -1;
	} else if ( !Q_stricmp( verb, "bars" ) ) {
		RC_Bars( (qboolean)( !numParts || Q_stricmp( parts[0], "off" ) ), numParts > 1 ? RC_Ms( parts[1] ) : RC_BAR_MS );
	} else if ( !Q_stricmp( verb, "hud" ) ) {
		rc.showHud = (qboolean)( !numParts || Q_stricmp( parts[0], "off" ) );
	} else {
		Com_DPrintf( "RP hook: cin wants start, path, move, cut, orbit, lookat, fov, bars, hud or end, not \"%s\"\n", verb );
	}
}

// cinematics sent through RPMod are let take over the camera
qboolean RC_CinematicsOn( void ) {
	return (qboolean)( rpc_cinematics && rpc_cinematics->integer );
}

void CL_RpCinematic_Hook( char **argv, int argc ) {
	if ( !rpc_cinematics->integer || cls.state != CA_ACTIVE )
		return;
	RC_Hook( argv, argc );
}

// a whole cin/... hook played on this screen only
static void RC_RunLocal( const char *hook ) {
	char buf[MAX_STRING_CHARS], *argv[RC_MAX_KEYS * 2 + 8];
	int argc;

	Q_strncpyz( buf, hook, sizeof( buf ) );
	argc = RC_SplitHook( buf, argv, ARRAY_LEN( argv ) );
	if ( argc > 1 )
		RC_Hook( argv + 1, argc - 1 );
}


/*
===============================================================================

Animations: one of JKA's forced on a player or an NPC, on this screen

Whoever plays a shot sends its animations in one hook,
  cmd:anim/<number> <ANIMATION> <from> <to> <p|r|h> <loop> <speed> <delay ms> <ms>[/...]
each <delay> ms after it comes (less than 0: it's already going), for <ms> (0: until
cmd:anim/stop, or the cinematic ends). It's the animation of that name in the model's
animation.cfg, its frames <from> to <to> (-1 its last), played (p), played backwards (r)
or held on <from> (h), round and round with loop 1, at <speed> percent. The frame is
worked out each time the model is drawn and set on its legs and torso, so nothing the
server sends changes it; let go, the cgame is told to start its own again.

===============================================================================
*/

#define RC_MAX_PLAYING		32
#define RC_MAX_CFGS			4
#define RC_MAX_ANIM_DEFS	2048
#define RC_ANIM_NEAR		64.0f		// a model drawn this near the entity is its
#define RC_HUMANOID_GLA		"models/players/_humanoid/_humanoid.gla"

static const char rcAnimModeChars[RC_NUM_AMODES + 1] = "prh";
static const char *rcAnimModeWords[RC_NUM_AMODES] = { "play", "reverse", "hold" };

// one playing on an entity
typedef struct {
	qboolean	active;
	qboolean	editor;				// a cinematic's timeline, set again every frame
	int			num;
	char		anim[48];
	int			from, to, mode;
	qboolean	loop;
	float		speed;
	int			start, end;			// cls.realtime; end 0 never
} rcPlaying_t;

// the animation.cfg next to a .gla, read once
static struct {
	char		gla[MAX_QPATH];
	rcAnimDef_t	*defs;
	int			count;
	int			used;
} rcCfgs[RC_MAX_CFGS];
static int rcCfgUse;

static rcPlaying_t rcPlaying[RC_MAX_PLAYING];
static int rcNumPlaying;
static qboolean rcFlip[MAX_GENTITIES];			// the cgame told to start its own animation again
static char rcEntGla[MAX_GENTITIES][MAX_QPATH];	// the skeleton each was last seen with

// going out once the server takes another command: the delays from when the shot began
static struct {
	qboolean	pending;
	rcAnim_t	anims[RC_MAX_ANIMS];
	int			count, shotMs, start;
} rcSend;

// the animation.cfg of a skeleton; -1 if it has none
static int RC_AnimCfg( const char *gla ) {
	char path[MAX_QPATH], *buf, *slash;
	const char *p;
	int oldest = 0;

	if ( !gla || !gla[0] )
		return -1;
	for ( int i = 0; i < RC_MAX_CFGS; i++ ) {
		if ( rcCfgs[i].used && !Q_stricmp( rcCfgs[i].gla, gla ) ) {
			rcCfgs[i].used = ++rcCfgUse;
			return rcCfgs[i].count ? i : -1;
		}
		if ( rcCfgs[i].used < rcCfgs[oldest].used )
			oldest = i;
	}
	// not there is kept too
	if ( rcCfgs[oldest].defs )
		Z_Free( rcCfgs[oldest].defs );
	memset( &rcCfgs[oldest], 0, sizeof( rcCfgs[0] ) );
	Q_strncpyz( rcCfgs[oldest].gla, gla, sizeof( rcCfgs[0].gla ) );
	rcCfgs[oldest].used = ++rcCfgUse;

	Q_strncpyz( path, gla, sizeof( path ) );
	slash = strrchr( path, '/' );
	if ( !slash )
		return -1;
	slash[1] = '\0';
	Q_strcat( path, sizeof( path ), "animation.cfg" );
	if ( FS_ReadFile( path, (void **)&buf ) <= 0 || !buf )
		return -1;
	rcCfgs[oldest].defs = (rcAnimDef_t *)Z_Malloc( RC_MAX_ANIM_DEFS * sizeof( rcAnimDef_t ), TAG_GENERAL, qtrue );
	p = buf;
	COM_BeginParseSession( path );
	while ( rcCfgs[oldest].count < RC_MAX_ANIM_DEFS ) {
		rcAnimDef_t *d = &rcCfgs[oldest].defs[rcCfgs[oldest].count];
		const char *token = COM_ParseExt( &p, qtrue );

		if ( !token[0] )
			break;
		if ( !isalpha( (unsigned char)token[0] ) ) {
			SkipRestOfLine( &p );
			continue;
		}
		Q_strncpyz( d->name, token, sizeof( d->name ) );
		d->first = atoi( COM_ParseExt( &p, qfalse ) );
		d->num = atoi( COM_ParseExt( &p, qfalse ) );
		COM_ParseExt( &p, qfalse );		// loop frames
		d->fps = (float)atof( COM_ParseExt( &p, qfalse ) );
		SkipRestOfLine( &p );
		if ( d->num > 0 && d->first >= 0 )
			rcCfgs[oldest].count++;
	}
	FS_FreeFile( buf );
	return rcCfgs[oldest].count ? oldest : -1;
}

const rcAnimDef_t *RC_FindAnim( int cfg, const char *name ) {
	if ( cfg < 0 )
		return NULL;
	for ( int i = 0; i < rcCfgs[cfg].count; i++ ) {
		if ( !Q_stricmp( rcCfgs[cfg].defs[i].name, name ) )
			return &rcCfgs[cfg].defs[i];
	}
	return NULL;
}

// the skeleton an entity was last drawn with, a humanoid's before it's been seen
int RC_EntCfg( int num ) {
	return RC_AnimCfg( num >= 0 && num < MAX_GENTITIES && rcEntGla[num][0] ? rcEntGla[num] : RC_HUMANOID_GLA );
}

// let go: the cgame starts its own animation again with the next snapshot
static void RC_Release( rcPlaying_t *p ) {
	if ( !p->active )
		return;
	p->active = qfalse;
	rcNumPlaying--;
	if ( p->num >= 0 && p->num < MAX_GENTITIES )
		rcFlip[p->num] = (qboolean)!rcFlip[p->num];
}

// several can wait their turn on someone: the one that started last plays, the editor's first
static rcPlaying_t *RC_StartAnim( int num, const char *anim, int from, int to, int mode, qboolean loop, int speed, int delay, int ms, qboolean editor ) {
	rcPlaying_t *p = NULL;

	if ( num < 0 || num >= MAX_GENTITIES || !anim[0] )
		return NULL;
	// the editor shows one at a time, in place of the one it showed
	for ( int i = 0; i < RC_MAX_PLAYING && editor; i++ ) {
		if ( rcPlaying[i].active && rcPlaying[i].num == num && rcPlaying[i].editor ) {
			p = &rcPlaying[i];
			break;
		}
	}
	for ( int i = 0; i < RC_MAX_PLAYING && !p; i++ ) {
		if ( !rcPlaying[i].active ) {
			p = &rcPlaying[i];
			rcNumPlaying++;
		}
	}
	if ( !p )
		return NULL;
	memset( p, 0, sizeof( *p ) );
	p->active = qtrue;
	p->editor = editor;
	p->num = num;
	Q_strncpyz( p->anim, anim, sizeof( p->anim ) );
	p->from = from;
	p->to = to;
	p->mode = Com_Clampi( 0, RC_NUM_AMODES - 1, mode );
	p->loop = loop;
	p->speed = Com_Clamp( 0.05f, 10.0f, speed / 100.0f );
	p->start = cls.realtime + delay;
	p->end = ms > 0 ? p->start + ms : 0;
	return p;
}

// every one playing let go; the editor's too, or not
static void RC_StopAnims( qboolean editorToo ) {
	for ( int i = 0; i < RC_MAX_PLAYING; i++ ) {
		if ( rcPlaying[i].active && ( editorToo || !rcPlaying[i].editor ) )
			RC_Release( &rcPlaying[i] );
	}
}

void RC_TimelineAnims( const int *nums, const rcAnim_t *const *anims, const int *elapsed, int count ) {
	for ( int j = 0; j < count; j++ ) {
		const rcAnim_t *a = anims[j];
		rcPlaying_t *p = RC_StartAnim( nums[j], a->anim, a->from, a->to, a->mode, a->loop, a->speed, 0, 0, qtrue );

		if ( p )
			p->start = cls.realtime - elapsed[j];
	}
	for ( int i = 0; i < RC_MAX_PLAYING; i++ ) {
		rcPlaying_t *p = &rcPlaying[i];
		int j = 0;

		if ( !p->active || !p->editor )
			continue;
		while ( j < count && nums[j] != p->num )
			j++;
		if ( j == count )
			RC_Release( p );
	}
}

void RC_TimelineClear( void ) {
	for ( int i = 0; i < RC_MAX_PLAYING; i++ ) {
		if ( rcPlaying[i].active && rcPlaying[i].editor )
			RC_Release( &rcPlaying[i] );
	}
}

// a skeleton's animations, to pick from
const rcAnimDef_t *RC_AnimDefs( int cfg, int *count ) {
	if ( cfg < 0 || cfg >= RC_MAX_CFGS ) {
		*count = 0;
		return NULL;
	}
	*count = rcCfgs[cfg].count;
	return rcCfgs[cfg].defs;
}

// where an entity is drawn now, near enough: where its walk has it, else the snapshot's
static qboolean RC_EntityOrigin( int num, vec3_t out ) {
	npcWalkInfo_t w;

	if ( num == clc.clientNum ) {
		VectorCopy( cl.snap.ps.origin, out );
		return qtrue;
	}
	if ( CL_NpcWalk_PoseOf( num, out ) )
		return qtrue;
	for ( int i = 0; CL_NpcWalk_GetWalk( i, &w ); i++ ) {
		if ( w.num == num ) {
			VectorCopy( w.pos, out );
			return qtrue;
		}
	}
	for ( int i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];

		if ( es->number == num ) {
			VectorCopy( es->pos.trBase, out );
			return qtrue;
		}
	}
	return qfalse;
}

// the frame in the file it's on now, and the last of its frames
static float RC_AnimFrame( const rcPlaying_t *p, const rcAnimDef_t *d, int *last ) {
	const int end = d->num - 1;
	const int from = Com_Clampi( 0, end, p->from );
	const int to = p->to < 0 ? end : Com_Clampi( from, end, p->to );
	float f = (float)from;

	if ( p->mode != RC_AM_HOLD && to > from ) {
		const float span = (float)( to - from );
		float pos = Q_max( 0, cls.realtime - p->start ) / 1000.0f * fabsf( d->fps ) * p->speed;

		pos = p->loop ? fmodf( pos, span ) : Q_min( pos, span );
		// an animation whose frames run backwards (a negative fps) plays them backwards already
		f = ( ( p->mode == RC_AM_REVERSE ) != ( d->fps < 0.0f ) ) ? to - pos : from + pos;
	}
	*last = d->first + to;
	return d->first + f;
}

// the legs and the torso on that frame, the next one lerped to as far as the animation goes
static void RC_SetFrame( CGhoul2Info_v &g2, float frame, int last ) {
	static const char *bones[] = { "model_root", "lower_lumbar", "Motion" };
	const int s = (int)floorf( frame ), e = s + 1 <= last ? s + 2 : s + 1;
	const float set = e == s + 2 ? frame : (float)s;
	const int time = re->G2API_GetTime( 0 );

	for ( size_t i = 0; i < ARRAY_LEN( bones ); i++ )
		re->G2API_SetBoneAnim( g2, 0, bones[i], s, e, BONE_ANIM_OVERRIDE_FREEZE, 0.001f, time, set, 0 );
}

// a model on its way to the renderer: if it's one of those playing an animation, on its frame
static void RC_AnimateModel( const refEntity_t *ent ) {
	CGhoul2Info_v &g2 = *(CGhoul2Info_v *)ent->ghoul2;
	const rcPlaying_t *pick = NULL;
	const rcAnimDef_t *def;
	float best = RC_ANIM_NEAR, frame;
	const char *gla;
	int last;

	if ( !re->G2API_HaveWeGhoul2Models( g2 ) )
		return;
	for ( int i = 0; i < RC_MAX_PLAYING; i++ ) {
		const rcPlaying_t *p = &rcPlaying[i];
		vec3_t org;

		if ( !p->active || cls.realtime < p->start || !RC_EntityOrigin( p->num, org ) )
			continue;
		// on the same one: the editor's first, it's what's being made, then the latest
		if ( pick && pick->num == p->num ) {
			if ( ( p->editor && !pick->editor ) || ( p->editor == pick->editor && p->start > pick->start ) )
				pick = p;
			continue;
		}
		if ( Distance( org, ent->origin ) < best ) {
			best = Distance( org, ent->origin );
			pick = p;
		}
	}
	if ( !pick )
		return;
	gla = re->G2API_GetGLAName( g2, 0 );
	if ( !gla || !gla[0] )
		return;
	// a weapon or a saber has no such animation: only the body takes it
	def = RC_FindAnim( RC_AnimCfg( gla ), pick->anim );
	if ( !def )
		return;
	Q_strncpyz( rcEntGla[pick->num], gla, sizeof( rcEntGla[0] ) );
	frame = RC_AnimFrame( pick, def, &last );
	RC_SetFrame( g2, frame, last );
}

// a snapshot's entity, and the player's own state, on their way to the cgame: one let go
// starts its animation again
void CL_RpCinematic_SnapEntity( entityState_t *es ) {
	if ( es->number >= 0 && es->number < MAX_GENTITIES && rcFlip[es->number] ) {
		es->legsFlip = (qboolean)!es->legsFlip;
		es->torsoFlip = (qboolean)!es->torsoFlip;
	}
}

void CL_RpCinematic_SnapPlayer( playerState_t *ps ) {
	if ( ps->clientNum >= 0 && ps->clientNum < MAX_CLIENTS && rcFlip[ps->clientNum] ) {
		ps->legsFlip = (qboolean)!ps->legsFlip;
		ps->torsoFlip = (qboolean)!ps->torsoFlip;
	}
}

// cmd:anim/<spec>/<spec>..., cmd:anim/stop
void CL_RpCinematic_AnimHook( char **argv, int argc ) {
	if ( !rpc_cinematics->integer || cls.state != CA_ACTIVE )
		return;
	if ( argc && !Q_stricmp( argv[0], "stop" ) ) {
		RC_StopAnims( qfalse );
		return;
	}
	for ( int i = 0; i < argc; i++ ) {
		char anim[48], mode = 'p';
		int num, from, to, loop, speed, delay, ms;

		if ( sscanf( argv[i], "%d %47s %d %d %c %d %d %d %d", &num, anim, &from, &to, &mode, &loop, &speed, &delay, &ms ) != 9 )
			continue;
		RC_StartAnim( num, anim, from, to, (int)( strchr( rcAnimModeChars, mode ) ? strchr( rcAnimModeChars, mode ) - rcAnimModeChars : 0 ),
			(qboolean)( loop != 0 ), speed, delay, ms, qfalse );
	}
}

// who a shot's animation is for: numbers, me, a player by their name, an NPC by its targetname,
// commas between; how many found
int RC_AnimTargets( const char *who, int *nums, int max ) {
	int count = 0;

	for ( const char *p = who; *p && count < max; ) {
		char item[MAX_QPATH];
		int len = 0, num = -1;

		while ( *p && *p != ',' ) {
			if ( len < (int)sizeof( item ) - 1 )
				item[len++] = *p;
			p++;
		}
		item[len] = '\0';
		if ( *p )
			p++;
		if ( !item[0] )
			continue;
		if ( !Q_stricmp( item, "me" ) ) {
			num = clc.clientNum;
		} else if ( Q_isanumber( item ) ) {
			num = atoi( item );
		} else {
			// a player, the colours left out, from three letters of it
			for ( int c = 0; c < MAX_CLIENTS && num < 0; c++ ) {
				const int ofs = cl.gameState.stringOffsets[CS_PLAYERS + c];
				char name[MAX_QPATH];

				if ( !ofs )
					continue;
				Q_strncpyz( name, Info_ValueForKey( cl.gameState.stringData + ofs, "n" ), sizeof( name ) );
				Q_StripColor( name );
				if ( name[0] && ( !Q_stricmp( name, item ) || ( strlen( item ) >= 3 && Q_stristr( name, item ) ) ) )
					num = c;
			}
			if ( num < 0 )
				num = CL_NpcWalk_Target( item );
		}
		if ( num >= 0 && num < MAX_GENTITIES )
			nums[count++] = num;
	}
	return count;
}

// how long it lasts: its own, or to the shot's end
int RC_AnimMs( const rcAnim_t *a, int shotMs ) {
	if ( a->ms > 0 )
		return a->ms;
	return shotMs > 0 ? Q_max( 500, shotMs - a->delay ) : 0;
}

// the shot's animations from now: on this screen, or sent to everyone once the server takes it
static void RC_PlayAnims( const rcAnim_t *anims, int count, int shotMs, qboolean local ) {
	if ( !count )
		return;
	if ( !local ) {
		rcSend.pending = qtrue;
		memcpy( rcSend.anims, anims, Q_min( count, RC_MAX_ANIMS ) * sizeof( anims[0] ) );
		rcSend.count = Q_min( count, RC_MAX_ANIMS );
		rcSend.shotMs = shotMs;
		rcSend.start = cls.realtime;
		return;
	}
	for ( int i = 0; i < count; i++ ) {
		const rcAnim_t *a = &anims[i];
		int nums[16];
		const int n = RC_AnimTargets( a->who, nums, ARRAY_LEN( nums ) );

		for ( int j = 0; j < n; j++ )
			RC_StartAnim( nums[j], a->anim, a->from, a->to, a->mode, a->loop, a->speed, a->delay, RC_AnimMs( a, shotMs ), qfalse );
	}
}

// the hook with them, each its delay from now, when the server takes another command
static void RC_RunAnimSend( void ) {
	char hook[MAX_STRING_CHARS];
	const int late = cls.realtime - rcSend.start;
	int parts = 0;

	if ( !rcSend.pending || cls.state != CA_ACTIVE || !CL_NpcWalk_CanSend() )
		return;
	rcSend.pending = qfalse;
	Q_strncpyz( hook, "rpnotify -1 1 cmd:anim", sizeof( hook ) );
	for ( int i = 0; i < rcSend.count; i++ ) {
		const rcAnim_t *a = &rcSend.anims[i];
		int nums[16];
		const int n = RC_AnimTargets( a->who, nums, ARRAY_LEN( nums ) );

		if ( !n )
			Com_Printf( S_COLOR_YELLOW "Camera shot: no one called %s to play %s\n", a->who, a->anim );
		for ( int j = 0; j < n; j++ ) {
			const char *spec = va( "/%i %s %i %i %c %i %i %i %i", nums[j], a->anim, a->from, a->to, rcAnimModeChars[Com_Clampi( 0, RC_NUM_AMODES - 1, a->mode )],
				a->loop ? 1 : 0, a->speed, a->delay - late, RC_AnimMs( a, rcSend.shotMs ) );

			if ( strlen( hook ) + strlen( spec ) > RC_MAX_HOOK )
				break;
			Q_strcat( hook, sizeof( hook ), spec );
			parts++;
		}
	}
	if ( parts )
		CL_AddReliableCommand( hook, qfalse );
}

// the frame, every frame: the ones over let go, and the hook sent
static void RC_RunAnims( void ) {
	RC_RunAnimSend();
	for ( int i = 0; i < RC_MAX_PLAYING && rcNumPlaying; i++ ) {
		rcPlaying_t *p = &rcPlaying[i];

		if ( !p->active )
			continue;
		if ( p->end && cls.realtime >= p->end ) {
			RC_Release( p );
			continue;
		}
		// one that started after it on the same one, and lasts as long, has taken over for good:
		// dropped, the cgame not told, the other playing on
		for ( int j = 0; j < RC_MAX_PLAYING; j++ ) {
			const rcPlaying_t *q = &rcPlaying[j];

			if ( q != p && q->active && q->num == p->num && q->editor == p->editor && q->start > p->start && q->start <= cls.realtime
				&& ( !q->end || ( p->end && p->end <= q->end ) ) ) {
				p->active = qfalse;
				rcNumPlaying--;
				break;
			}
		}
	}
}

// the map's going: nothing plays, nothing's to start again
static void RC_ResetAnims( void ) {
	memset( rcPlaying, 0, sizeof( rcPlaying ) );
	rcNumPlaying = 0;
	memset( rcFlip, 0, sizeof( rcFlip ) );
	memset( rcEntGla, 0, sizeof( rcEntGla ) );
	rcSend.pending = qfalse;
}

/*
===============================================================================

Shot files

===============================================================================
*/

static qboolean RC_ShotNameChar( int ch ) {
	return (qboolean)( isalnum( ch ) || ch == '_' || ch == '-' );
}

static const char *RC_ShotPath( const char *name ) {
	return va( "%s/%s%s", RC_SHOT_DIR, name, RC_SHOT_EXT );
}

// the Storyteller asks for its steps' shots every frame: the files read, kept until one is saved
#define RC_SHOT_CACHE	16

static struct {
	char	name[MAX_QPATH];
	char	hook[RC_MAX_HOOK + 1];
	int		ms;
	int		used;
} rcShotCache[RC_SHOT_CACHE];

static int rcShotCacheUse;

static const char *RC_WalkCommand( const rcWalk_t *w, qboolean local ) {
	return va( "npcmanager walk %s %s %s%s", w->npc, w->route, w->run ? "run" : "walk", local ? " local" : "" );
}

// the shot's walks set off, each its delay from now; local ones on this screen only
static void RC_QueueWalks( const rcWalk_t *walks, int count, qboolean local ) {
	for ( int i = 0; i < count && rcNumQueued < RC_MAX_QUEUED; i++ ) {
		if ( !walks[i].npc[0] || !walks[i].route[0] )
			continue;
		Q_strncpyz( rcQueue[rcNumQueued].cmd, RC_WalkCommand( &walks[i], local ), sizeof( rcQueue[0].cmd ) );
		rcQueue[rcNumQueued].at = cls.realtime + walks[i].delay;
		rcQueue[rcNumQueued].local = local;
		rcNumQueued++;
	}
}

// the walks due: local ones at once, the others one at a time as the server takes them
static void RC_RunQueue( void ) {
	for ( int i = 0; i < rcNumQueued; ) {
		if ( cls.realtime < rcQueue[i].at || ( !rcQueue[i].local && ( cls.state != CA_ACTIVE || !CL_NpcWalk_CanSend() ) ) ) {
			i++;
			continue;
		}
		Cmd_ExecuteString( rcQueue[i].cmd );
		rcNumQueued--;
		memmove( &rcQueue[i], &rcQueue[i + 1], ( rcNumQueued - i ) * sizeof( rcQueue[0] ) );
	}
}

// a preview's walks called off: the ones still to go, and the NPCs walking back where they are
static void RC_StopLocalWalks( const rcWalk_t *walks, int count ) {
	for ( int i = 0; i < rcNumQueued; ) {
		if ( rcQueue[i].local ) {
			rcNumQueued--;
			memmove( &rcQueue[i], &rcQueue[i + 1], ( rcNumQueued - i ) * sizeof( rcQueue[0] ) );
		} else {
			i++;
		}
	}
	for ( int i = 0; i < count; i++ ) {
		if ( walks[i].npc[0] && cls.state == CA_ACTIVE )
			Cmd_ExecuteString( va( "npcmanager walk %s stop local", walks[i].npc ) );
	}
}

// a shot file's walk lines: walk <npc> <route> walk|run <seconds>
int RC_ReadWalks( const char *name, rcWalk_t *walks, int max ) {
	char *buf, *line, *next;
	int count = 0;

	if ( FS_ReadFile( RC_ShotPath( name ), (void **)&buf ) <= 0 || !buf )
		return 0;
	for ( line = buf; line && *line && count < max; line = next ) {
		char npc[MAX_QPATH], route[MAX_QPATH], pace[16];
		float seconds = 0;

		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		if ( Q_stricmpn( line, "walk ", 5 ) || sscanf( line + 5, "%63s %63s %15s %f", npc, route, pace, &seconds ) < 2 )
			continue;
		Q_strncpyz( walks[count].npc, npc, sizeof( walks[0].npc ) );
		Q_strncpyz( walks[count].route, route, sizeof( walks[0].route ) );
		walks[count].run = (qboolean)!Q_stricmp( pace, "run" );
		walks[count].delay = RC_SecondsMs( seconds );
		count++;
	}
	FS_FreeFile( buf );
	return count;
}


// a shot file's animation lines:
//   anim <who> <ANIMATION> <from> <to> play|reverse|hold loop|once <speed %> <seconds in> <seconds, 0 to the end>
int RC_ReadAnims( const char *name, rcAnim_t *anims, int max ) {
	char *buf, *line, *next;
	int count = 0;

	if ( FS_ReadFile( RC_ShotPath( name ), (void **)&buf ) <= 0 || !buf )
		return 0;
	for ( line = buf; line && *line && count < max; line = next ) {
		char who[MAX_QPATH], anim[48], mode[16] = "play", loop[16] = "once";
		float delay = 0, seconds = 0;
		int from = 0, to = -1, speed = 100;
		rcAnim_t *a = &anims[count];

		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		if ( Q_stricmpn( line, "anim ", 5 ) || sscanf( line + 5, "%63s %47s %d %d %15s %15s %d %f %f", who, anim, &from, &to, mode, loop, &speed, &delay, &seconds ) < 2 )
			continue;
		memset( a, 0, sizeof( *a ) );
		Q_strncpyz( a->who, who, sizeof( a->who ) );
		Q_strncpyz( a->anim, anim, sizeof( a->anim ) );
		a->from = from;
		a->to = to;
		a->mode = !Q_stricmp( mode, "reverse" ) ? RC_AM_REVERSE : !Q_stricmp( mode, "hold" ) ? RC_AM_HOLD : RC_AM_PLAY;
		a->loop = (qboolean)!Q_stricmp( loop, "loop" );
		a->speed = Com_Clampi( 5, 1000, speed );
		a->delay = RC_SecondsMs( delay );
		a->ms = RC_SecondsMs( seconds );
		count++;
	}
	FS_FreeFile( buf );
	return count;
}

static const char *RC_AnimLine( const rcAnim_t *a ) {
	return va( "anim %s %s %i %i %s %s %i %g %g\n", a->who, a->anim, a->from, a->to, rcAnimModeWords[Com_Clampi( 0, RC_NUM_AMODES - 1, a->mode )],
		a->loop ? "loop" : "once", a->speed, RC_Seconds( a->delay ), RC_Seconds( a->ms ) );
}

// the saved shot's NPCs set off, each when the shot says, and its animations play: sent for
// everyone, or local
void CL_RpCinematic_ShotWalks( const char *name, qboolean local ) {
	rcWalk_t walks[RC_MAX_WALKS];
	rcAnim_t anims[RC_MAX_ANIMS];
	char hook[MAX_STRING_CHARS];
	rcShot_t shot;
	const int numAnims = RC_ReadAnims( name, anims, RC_MAX_ANIMS );

	RC_QueueWalks( walks, RC_ReadWalks( name, walks, RC_MAX_WALKS ), local );
	if ( numAnims )
		RC_PlayAnims( anims, numAnims, RC_ReadShot( name, hook, sizeof( hook ), &shot ) ? RC_ShotMs( &shot ) : 0, local );
}

static void RC_ForgetShots( void ) {
	memset( rcShotCache, 0, sizeof( rcShotCache ) );
}

// a saved shot as its hook and as a shot; qfalse if there is none, or it isn't one
qboolean RC_ReadShot( const char *name, char *hook, int size, rcShot_t *shot ) {
	char *buf, copy[MAX_STRING_CHARS], *argv[RC_MAX_KEYS * 2 + 8];
	int len, argc;

	if ( !name[0] )
		return qfalse;
	for ( const char *c = name; *c; c++ ) {
		if ( !RC_ShotNameChar( *c ) )
			return qfalse;
	}
	len = FS_ReadFile( RC_ShotPath( name ), (void **)&buf );
	if ( len <= 0 || !buf )
		return qfalse;
	Q_strncpyz( copy, buf, Q_min( (int)sizeof( copy ), len + 1 ) );
	FS_FreeFile( buf );

	// the first line that isn't a comment
	char *line = copy;
	while ( *line && ( *line == '/' && line[1] == '/' ) ) {
		line = strchr( line, '\n' );
		line = line ? line + 1 : copy + strlen( copy );
	}
	char *end = strpbrk( line, "\r\n" );
	if ( end )
		*end = '\0';
	if ( Q_stricmpn( line, "cin/", 4 ) || strstr( line, "//" ) )
		return qfalse;
	Q_strncpyz( hook, line, size );
	argc = RC_SplitHook( line, argv, ARRAY_LEN( argv ) );
	return RC_ParseShot( argv + 1, argc - 1, shot, vec3_origin );
}

// a hook going out: the one its focus follows by their number, every screen knowing who that
// is; a name nobody here goes by is left for the others to look for
static void RC_ResolveFocusWho( char *hook, int size ) {
	char *part = (char *)Q_stristr( hook, "/dof " ), *who, *end, rest[MAX_STRING_CHARS], name[MAX_QPATH];
	float v[3];
	int n = 0, num, len;

	if ( !part || sscanf( part + 5, "%f %f %f %n", &v[0], &v[1], &v[2], &n ) < 3 || n <= 0 )
		return;
	who = part + 5 + n;
	end = strchr( who, '/' );
	len = end ? (int)( end - who ) : (int)strlen( who );
	if ( len <= 0 || len >= (int)sizeof( name ) )
		return;
	Q_strncpyz( name, who, len + 1 );
	if ( Q_isanumber( name ) || RC_AnimTargets( name, &num, 1 ) < 1 )
		return;
	Q_strncpyz( rest, end ? end : "", sizeof( rest ) );
	Com_sprintf( who, size - (int)( who - hook ), "%i%s", num, rest );
}

// a saved shot's hook and how long it lasts, for /cincam play and the Storyteller
qboolean CL_RpCinematic_ShotHook( const char *name, char *hook, int size, int *ms ) {
	rcShot_t shot;
	int oldest = 0;

	for ( int i = 0; i < RC_SHOT_CACHE; i++ ) {
		if ( rcShotCache[i].used && !Q_stricmp( rcShotCache[i].name, name ) ) {
			rcShotCache[i].used = ++rcShotCacheUse;
			if ( !rcShotCache[i].hook[0] )
				return qfalse;
			Q_strncpyz( hook, rcShotCache[i].hook, size );
			RC_ResolveFocusWho( hook, size );
			if ( ms )
				*ms = rcShotCache[i].ms;
			return qtrue;
		}
		if ( rcShotCache[i].used < rcShotCache[oldest].used )
			oldest = i;
	}

	// not there is kept too
	Q_strncpyz( rcShotCache[oldest].name, name, sizeof( rcShotCache[oldest].name ) );
	rcShotCache[oldest].used = ++rcShotCacheUse;
	rcShotCache[oldest].hook[0] = '\0';
	if ( !RC_ReadShot( name, rcShotCache[oldest].hook, sizeof( rcShotCache[oldest].hook ), &shot ) ) {
		rcShotCache[oldest].hook[0] = '\0';
		return qfalse;
	}
	rcShotCache[oldest].ms = RC_ShotMs( &shot );
	Q_strncpyz( hook, rcShotCache[oldest].hook, size );
	RC_ResolveFocusWho( hook, size );
	if ( ms )
		*ms = rcShotCache[oldest].ms;
	return qtrue;
}

/*
===============================================================================

On the way to the renderer

===============================================================================
*/

qboolean CL_RpCinematic_Active( void ) {
	return rc.active;
}

// the HUD hidden, chat box and all: what's said shows as subtitles instead
qboolean CL_RpCinematic_Subtitles( void ) {
	return (qboolean)( rc.active && !rc.showHud );
}

// the editor's own camera is what's seen, not a preview
static qboolean CE_Viewing( void ) {
	return CM_EditorViewing();
}

// the cgame's 2D is left out while the HUD is hidden, but what covers the whole screen,
// fades and overlays, still shows. Text is never the whole screen
qboolean CL_RpCinematic_Hide2D( float x, float y, float w, float h ) {
	if ( !CE_Viewing() && ( !rc.active || rc.showHud ) )
		return qfalse;
	return (qboolean)!( x <= 0.0f && y <= 0.0f && x + w >= SCREEN_WIDTH && y + h >= SCREEN_HEIGHT );
}

// how far off what the camera looks at is: the world, or a player or NPC in the way
static float RC_LookDistance( const vec3_t org, const vec3_t ang ) {
	vec3_t forward, end;
	trace_t tr;
	float best;

	AngleVectors( ang, forward, NULL, NULL );
	VectorMA( org, 8192.0f, forward, end );
	CM_BoxTrace( &tr, org, end, vec3_origin, vec3_origin, 0, MASK_SOLID, qfalse );
	best = tr.fraction * 8192.0f;
	// the player too, drawn in third person, though the snapshot doesn't list them
	for ( int i = -1; i < cl.snap.numEntities; i++ ) {
		const entityState_t *es = i >= 0 ? &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )] : NULL;
		vec3_t to, mid;
		float along;

		if ( es && es->eType != ET_PLAYER && es->eType != ET_NPC )
			continue;
		VectorCopy( es ? es->pos.trBase : cl.snap.ps.origin, mid );
		mid[2] += 16.0f;
		VectorSubtract( mid, org, to );
		along = DotProduct( to, forward );
		if ( along <= 8.0f || along >= best )
			continue;
		VectorMA( to, -along, forward, to );
		if ( VectorLength( to ) < 36.0f )
			best = along;
	}
	return best;
}

// where someone is, to keep in focus: a little over their middle; qfalse if they aren't here
qboolean RC_FocusTarget( const char *who, vec3_t out ) {
	int num;

	if ( !who[0] || RC_AnimTargets( who, &num, 1 ) < 1 || !RC_EntityOrigin( num, out ) )
		return qfalse;
	out[2] += RC_DOF_FOLLOW_UP;
	return qtrue;
}

// a spot's depth of field: its own, or the last one set before it, or the shot's
void RC_KeyDof( const rcShot_t *s, int key, float *focus, float *range, float *blur ) {
	*focus = s->dofFocus;
	*range = s->dofRange;
	*blur = s->dofBlur;
	for ( int i = 0; i <= key && i < s->numKeys; i++ ) {
		if ( s->keys[i].dofSet ) {
			*focus = s->keys[i].dofFocus;
			*range = s->keys[i].dofRange;
			*blur = s->keys[i].dofBlur;
		}
	}
}

// how far off a focus is from org: as set, what's looked at, or the one followed (what's
// looked at while they aren't here)
static float RC_FocusDistance( const rcShot_t *s, float focus, const vec3_t org, const vec3_t ang ) {
	vec3_t at;

	if ( focus > 0.0f )
		return focus;
	if ( focus < 0.0f && RC_FocusTarget( s->dofWho, at ) )
		return Q_max( 1.0f, Distance( org, at ) );
	return RC_LookDistance( org, ang );
}

// where ms into the shot is among its spots: between prev and next, frac of the way, eased as
// the camera is
static void RC_ShotPlace( const rcShot_t *s, int ms, int *prev, int *next, float *frac ) {
	const int total = RC_ShotMs( s );
	float u, from = 0;
	int i;

	*prev = *next = Q_max( 0, s->numKeys - 1 );
	*frac = 0.0f;
	if ( s->orbit || s->numKeys < 2 || total <= 0 )
		return;
	u = RC_Ease( (float)ms / total, s->linear ) * total;
	for ( i = 1; i < s->numKeys - 1 && u > from + s->keys[i].ms; i++ )
		from += s->keys[i].ms;
	*prev = i - 1;
	*next = i;
	*frac = s->keys[i].ms > 0 ? Com_Clamp( 0.0f, 1.0f, ( u - from ) / s->keys[i].ms ) : 1.0f;
}

// the depth of field the camera at org has frac of the way from spot prev's to next's; a focus
// on what's looked at or on someone is eased onto, not snapped. qfalse if none
qboolean RC_DepthOfField( const rcShot_t *s, int prev, int next, float frac, const vec3_t org, const vec3_t ang,
	float *now, int *time ) {
	float f0, r0, b0, f1, r1, b1, to, range, blur;

	if ( !s->dof || !re->ext.SetDepthOfField )
		return qfalse;
	RC_KeyDof( s, prev, &f0, &r0, &b0 );
	RC_KeyDof( s, next, &f1, &r1, &b1 );
	frac = Com_Clamp( 0.0f, 1.0f, frac );
	range = r0 + ( r1 - r0 ) * frac;
	blur = b0 + ( b1 - b0 ) * frac;
	if ( blur <= 0.0f )
		return qfalse;
	to = RC_FocusDistance( s, f0, org, ang );
	if ( frac > 0.0f && f1 != f0 )
		to += ( RC_FocusDistance( s, f1, org, ang ) - to ) * frac;

	if ( ( f0 > 0.0f && f1 > 0.0f ) || *now <= 0.0f ) {
		*now = to;
	} else {
		const float dt = Com_Clamp( 0.0f, 0.2f, ( cls.realtime - *time ) * 0.001f );
		const float rate = ( f0 == 0.0f || f1 == 0.0f ) ? RC_DOF_FOCUS_RATE : RC_DOF_FOLLOW_RATE;

		*now += ( to - *now ) * Com_Clamp( 0.0f, 1.0f, dt * rate );
	}
	*time = cls.realtime;
	re->ext.SetDepthOfField( *now, range, blur );
	return qtrue;
}

// where a moving focus is ms into its shot: from key to key, held before the first and after the
// last
void RC_FocusPoint( const rcShot_t *s, int ms, vec3_t at, float *range ) {
	int i;

	if ( !s->numFocus ) {
		VectorClear( at );
		*range = s->dofRange;
		return;
	}
	for ( i = 0; i < s->numFocus - 1 && s->focus[i + 1].ms <= ms; i++ )
		;
	VectorCopy( s->focus[i].org, at );
	*range = s->focus[i].range;
	if ( i + 1 < s->numFocus && ms > s->focus[i].ms ) {
		const rcFocus_t *a = &s->focus[i], *b = &s->focus[i + 1];
		const float f = b->ms > a->ms ? Com_Clamp( 0.0f, 1.0f, (float)( ms - a->ms ) / ( b->ms - a->ms ) ) : 1.0f;

		for ( int j = 0; j < 3; j++ )
			at[j] = a->org[j] + ( b->org[j] - a->org[j] ) * f;
		*range = a->range + ( b->range - a->range ) * f;
	}
}

// the depth of field ms into the shot
qboolean RC_ShotDepthOfField( const rcShot_t *s, int ms, const vec3_t org, const vec3_t ang, float *now, int *time ) {
	int prev, next;
	float frac;

	// on a spot that moves: as deep into the picture as it is
	if ( s->dof && s->dofFocus == RC_DOF_POINT && s->numFocus ) {
		vec3_t at, fwd, d;
		float range;

		if ( !re->ext.SetDepthOfField || s->dofBlur <= 0.0f )
			return qfalse;
		RC_FocusPoint( s, ms, at, &range );
		AngleVectors( ang, fwd, NULL, NULL );
		VectorSubtract( at, org, d );
		*now = Q_max( 1.0f, DotProduct( d, fwd ) );
		*time = cls.realtime;
		re->ext.SetDepthOfField( *now, Q_max( 8.0f, range ), s->dofBlur );
		return qtrue;
	}

	RC_ShotPlace( s, ms, &prev, &next, &frac );
	return RC_DepthOfField( s, prev, next, frac, org, ang, now, time );
}

void RC_NoDepthOfField( void ) {
	if ( re->ext.SetDepthOfField )
		re->ext.SetDepthOfField( 0.0f, 1.0f, 0.0f );
}

/*
===============================================================================

Filters: a look over the whole picture, like a video editor's

===============================================================================
*/

static const char *rcFilterWords[RC_NUM_FILTERS] = { "none", "bw", "noir", "sepia", "oldfilm", "nightvision", "thermal", "cctv",
	"vhs", "hologram", "warm", "cold", "faded", "negative" };
static const char *rcFilterNames[RC_NUM_FILTERS] = { "None", "Black and white", "Noir", "Sepia", "Old film", "Night vision",
	"Thermal", "Security camera", "VHS tape", "Hologram", "Warm", "Cold", "Faded", "Negative" };

static int RC_FilterByWord( const char *word ) {
	for ( int i = 0; i < RC_NUM_FILTERS; i++ ) {
		if ( !Q_stricmp( word, rcFilterWords[i] ) )
			return i;
	}
	return RC_FI_NONE;
}

static const char *RC_FilterWord( int type ) {
	return rcFilterWords[Com_Clampi( 0, RC_NUM_FILTERS - 1, type )];
}

const char *RC_FilterName( int type ) {
	return rcFilterNames[Com_Clampi( 0, RC_NUM_FILTERS - 1, type )];
}

// the colours mixed by a after what the filter does already
static void RC_FilterMix( screenFilter_t *f, const float a[3][4] ) {
	float m[3][4];

	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			m[i][j] = a[i][0] * f->matrix[0][j] + a[i][1] * f->matrix[1][j] + a[i][2] * f->matrix[2][j];
			if ( j == 3 )
				m[i][j] += a[i][3];
		}
	}
	memcpy( f->matrix, m, sizeof( m ) );
}

// how colourful: 0 grey, 1 as it is
static void RC_FilterSaturation( screenFilter_t *f, float sat ) {
	const float lr = 0.299f * ( 1.0f - sat ), lg = 0.587f * ( 1.0f - sat ), lb = 0.114f * ( 1.0f - sat );
	const float a[3][4] = { { lr + sat, lg, lb, 0 }, { lr, lg + sat, lb, 0 }, { lr, lg, lb + sat, 0 } };

	RC_FilterMix( f, a );
}

// darks darker and lights lighter round pivot, above 1; JKA's maps are dark, so their middle
// is lower than half
static void RC_FilterContrast( screenFilter_t *f, float c, float pivot ) {
	const float o = pivot * ( 1.0f - c );
	const float a[3][4] = { { c, 0, 0, o }, { 0, c, 0, o }, { 0, 0, c, o } };

	RC_FilterMix( f, a );
}

// each colour times its own, and the black lifted to lift
static void RC_FilterTint( screenFilter_t *f, float r, float g, float b, float lift ) {
	const float k = 1.0f - lift;
	const float a[3][4] = { { r * k, 0, 0, lift }, { 0, g * k, 0, lift }, { 0, 0, b * k, lift } };

	RC_FilterMix( f, a );
}

static void RC_FilterRamp( screenFilter_t *f, float gain, const byte colors[5][3] ) {
	f->ramp = 1.0f;
	f->rampGain = gain;
	memcpy( f->rampColors, colors, sizeof( f->rampColors ) );
}

// a filter's look now: some flicker as time goes
static void RC_BuildFilter( int type, screenFilter_t *f ) {
	static const byte night[5][3] = { { 0, 8, 0 }, { 10, 70, 10 }, { 40, 170, 40 }, { 140, 255, 120 }, { 230, 255, 220 } };
	static const byte thermal[5][3] = { { 10, 0, 40 }, { 80, 0, 150 }, { 220, 30, 60 }, { 255, 170, 0 }, { 255, 255, 210 } };
	static const byte holo[5][3] = { { 0, 10, 30 }, { 10, 60, 130 }, { 40, 140, 230 }, { 130, 210, 255 }, { 220, 245, 255 } };
	static const float sepia[3][4] = { { 0.393f, 0.769f, 0.189f, 0 }, { 0.349f, 0.686f, 0.168f, 0 }, { 0.272f, 0.534f, 0.131f, 0 } };
	const float t = cls.realtime * 0.001f;
	float flicker = 0.0f;

	memset( f, 0, sizeof( *f ) );
	for ( int i = 0; i < 3; i++ )
		f->matrix[i][i] = 1.0f;
	f->rampGain = 1.0f;
	for ( int i = 0; i < 5; i++ )
		f->rampColors[i][0] = f->rampColors[i][1] = f->rampColors[i][2] = (byte)( i * 255 / 4 );

	switch ( type ) {
	case RC_FI_BW:
		RC_FilterSaturation( f, 0.0f );
		break;
	case RC_FI_NOIR:
		RC_FilterSaturation( f, 0.0f );
		RC_FilterContrast( f, 1.5f, 0.25f );
		f->vignette = 0.5f;
		f->grain = 0.05f;
		break;
	case RC_FI_SEPIA:
		RC_FilterMix( f, sepia );
		f->vignette = 0.25f;
		break;
	case RC_FI_OLDFILM:
		RC_FilterMix( f, sepia );
		RC_FilterContrast( f, 1.1f, 0.25f );
		f->grain = 0.12f;
		f->vignette = 0.55f;
		flicker = 0.05f * sinf( t * 31.0f ) + 0.04f * sinf( t * 13.7f );
		break;
	case RC_FI_NIGHT:
		RC_FilterRamp( f, 2.6f, night );
		f->grain = 0.12f;
		f->vignette = 1.0f;
		f->scanlines = 0.12f;
		f->scanlineCount = 240.0f;
		break;
	case RC_FI_THERMAL:
		RC_FilterRamp( f, 1.25f, thermal );
		f->grain = 0.03f;
		break;
	case RC_FI_CCTV:
		RC_FilterSaturation( f, 0.25f );
		RC_FilterTint( f, 0.9f, 1.0f, 0.95f, 0.0f );
		RC_FilterContrast( f, 1.15f, 0.25f );
		f->scanlines = 0.25f;
		f->scanlineCount = 200.0f;
		f->grain = 0.08f;
		f->vignette = 0.5f;
		f->aberration = 0.4f;
		break;
	case RC_FI_VHS:
		RC_FilterSaturation( f, 1.25f );
		RC_FilterTint( f, 1.05f, 1.0f, 0.95f, 0.04f );
		f->aberration = 1.6f;
		f->scanlines = 0.15f;
		f->scanlineCount = 240.0f;
		f->grain = 0.06f;
		f->vignette = 0.3f;
		break;
	case RC_FI_HOLOGRAM:
		RC_FilterRamp( f, 1.2f, holo );
		f->scanlines = 0.35f;
		f->scanlineCount = 160.0f;
		f->grain = 0.06f;
		f->aberration = 0.6f;
		flicker = 0.06f * sinf( t * 40.0f ) + 0.04f * sinf( t * 9.3f );
		break;
	case RC_FI_WARM:
		RC_FilterTint( f, 1.12f, 1.0f, 0.82f, 0.0f );
		RC_FilterSaturation( f, 1.1f );
		break;
	case RC_FI_COLD:
		RC_FilterTint( f, 0.85f, 0.98f, 1.15f, 0.0f );
		RC_FilterSaturation( f, 0.9f );
		break;
	case RC_FI_FADED:
		RC_FilterSaturation( f, 0.55f );
		RC_FilterContrast( f, 0.9f, 0.25f );
		RC_FilterTint( f, 1.05f, 1.0f, 0.92f, 0.1f );
		f->vignette = 0.3f;
		break;
	case RC_FI_NEGATIVE:
		{
			const float a[3][4] = { { -1, 0, 0, 1 }, { 0, -1, 0, 1 }, { 0, 0, -1, 1 } };

			RC_FilterMix( f, a );
		}
		break;
	default:
		break;
	}
	// a flicker: the whole picture lighter and darker
	if ( flicker != 0.0f ) {
		for ( int i = 0; i < 3; i++ ) {
			for ( int j = 0; j < 4; j++ )
				f->matrix[i][j] *= 1.0f + flicker;
		}
		f->rampGain *= 1.0f + flicker;
	}
}

// the filter over the next world scene; qfalse if none
qboolean RC_Filter( int type, int strength ) {
	screenFilter_t f;

	if ( type <= RC_FI_NONE || type >= RC_NUM_FILTERS || strength <= 0 || !re->ext.SetScreenFilter )
		return qfalse;
	RC_BuildFilter( type, &f );
	f.strength = Com_Clampi( 0, 100, strength ) / 100.0f;
	re->ext.SetScreenFilter( &f );
	return qtrue;
}

void RC_NoFilter( void ) {
	if ( re->ext.SetScreenFilter )
		re->ext.SetScreenFilter( NULL );
}

// a strip of a wipe, from a0 to a1 of the way across from the side it comes from
static void RC_WipeStrip( int type, float a0, float a1 ) {
	a0 = Com_Clamp( 0.0f, 1.0f, a0 );
	a1 = Com_Clamp( 0.0f, 1.0f, a1 );
	if ( a1 <= a0 )
		return;
	switch ( type ) {
	case RC_TR_LEFT:
		re->DrawStretchPic( a0 * SCREEN_WIDTH, 0, ( a1 - a0 ) * SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0, cls.whiteShader );
		break;
	case RC_TR_RIGHT:
		re->DrawStretchPic( ( 1.0f - a1 ) * SCREEN_WIDTH, 0, ( a1 - a0 ) * SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0, cls.whiteShader );
		break;
	case RC_TR_TOP:
		re->DrawStretchPic( 0, a0 * SCREEN_HEIGHT, SCREEN_WIDTH, ( a1 - a0 ) * SCREEN_HEIGHT, 0, 0, 0, 0, cls.whiteShader );
		break;
	default:
		re->DrawStretchPic( 0, ( 1.0f - a1 ) * SCREEN_HEIGHT, SCREEN_WIDTH, ( a1 - a0 ) * SCREEN_HEIGHT, 0, 0, 0, 0, cls.whiteShader );
		break;
	}
}

// a transition's colour over the screen: faded in by how far it is, or wiped across, its edge
// sweeping from the side; coming in, the picture is behind the edge, going out, the colour is
static void RC_DrawCover( const rcTrans_t *tr, float progress, qboolean in ) {
	const int steps = 12;
	const float soft = RC_WIPE_SOFT, step = soft / steps;
	// the edge, from just before the side to just past the other one, for its soft part to
	// come on and leave the screen too
	const float e = progress * ( 1.0f + soft ) - soft;
	vec4_t color = { tr->color[0] / 255.0f, tr->color[1] / 255.0f, tr->color[2] / 255.0f, 1.0f };

	if ( tr->type == RC_TR_FADE ) {
		color[3] = in ? 1.0f - progress : progress;
		if ( color[3] <= 0.0f )
			return;
		re->SetColor( color );
		re->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0, cls.whiteShader );
		re->SetColor( NULL );
		return;
	}
	re->SetColor( color );
	if ( in )
		RC_WipeStrip( tr->type, e + soft, 1.0f );
	else
		RC_WipeStrip( tr->type, 0.0f, e );
	for ( int i = 0; i < steps; i++ ) {
		color[3] = in ? ( i + 0.5f ) / steps : 1.0f - ( i + 0.5f ) / steps;
		re->SetColor( color );
		RC_WipeStrip( tr->type, e + i * step, e + ( i + 1 ) * step );
	}
	re->SetColor( NULL );
}

// the shot playing coming in or going out, ms into it
void RC_DrawTransitions( const rcShot_t *shot, int ms ) {
	const int total = RC_ShotMs( shot );

	if ( shot->in.type != RC_TR_CUT && shot->in.ms > 0 && ms < shot->in.ms )
		RC_DrawCover( &shot->in, RC_Ease( (float)ms / shot->in.ms, qfalse ), qtrue );
	else if ( shot->out.type != RC_TR_CUT && shot->out.ms > 0 && ms >= total - shot->out.ms )
		RC_DrawCover( &shot->out, RC_Ease( (float)( ms - ( total - shot->out.ms ) ) / shot->out.ms, qfalse ), qfalse );
}

// the cgame's main view: the player's is kept to start from, the camera's drawn instead.
// qtrue if it was drawn here, or left out
qboolean CL_RpCinematic_RenderScene( const refdef_t *fd ) {
	refdef_t view;

	// the HUD's 3D bits, heads and items
	if ( fd->rdflags & RDF_NOWORLDMODEL )
		return (qboolean)( CE_Viewing() || ( rc.active && !rc.showHud ) );
	if ( fd->rdflags & RDF_AUTOMAP )
		return qfalse;

	if ( !rc.active && !( fd->rdflags & RDF_SKYBOXPORTAL ) ) {
		rc.haveView = qtrue;
		VectorCopy( fd->vieworg, rc.viewOrg );
		VectorCopy( fd->viewangles, rc.viewAng );
		rc.viewFovY = fd->fov_y;
	}
	if ( CE_Viewing() ) {
		CM_EditorRenderScene( fd );
		return qtrue;
	}
	if ( !rc.active && CM_RecordView( fd ) )
		return qtrue;
	if ( !rc.active )
		return qfalse;

	RC_Update();
	view = *fd;
	view.fov_y = rc.fovY;
	view.fov_x = RC_FovX( rc.fovY, (float)fd->width, (float)fd->height );
	VectorCopy( rc.ang, view.viewangles );
	AnglesToAxis( rc.ang, view.viewaxis );
	// the sky portal keeps its own origin but looks the way the camera does
	if ( !( fd->rdflags & RDF_SKYBOXPORTAL ) ) {
		VectorCopy( rc.org, view.vieworg );
		view.viewContents = CM_PointContents( rc.org, 0 );
		// the snapshot's area mask is from the player's position, the camera can be anywhere
		memset( view.areamask, 0, sizeof( view.areamask ) );
		{
			// the shot's, where it's got to; held after it, as it ended
			const int ms = rc.playing ? cls.realtime - rc.start : RC_ShotMs( &rc.shot );
			const qboolean dof = (qboolean)( rc.dof && RC_ShotDepthOfField( &rc.shot, ms, rc.org, rc.ang, &rc.dofNow, &rc.dofTime ) );
			const qboolean filter = (qboolean)( rc.filter && RC_Filter( rc.shot.filter, rc.shot.filterStrength ) );

			re->RenderScene( &view );
			if ( dof )
				RC_NoDepthOfField();
			if ( filter )
				RC_NoFilter();
			return qtrue;
		}
	}
	re->RenderScene( &view );
	return qtrue;
}

// the camera, for the effects system to cull against
qboolean CL_RpCinematic_Camera( vec3_t origin, vec3_t angles ) {
	if ( CE_Viewing() ) {
		float fov;

		CM_EditorCamera( origin, angles, &fov );
		return qtrue;
	}
	if ( !rc.active && CM_RecordCamera( origin, angles ) )
		return qtrue;
	if ( !rc.active )
		return qfalse;
	RC_Update();
	VectorCopy( rc.org, origin );
	VectorCopy( rc.ang, angles );
	return qtrue;
}

// what the cgame adds to the scene, NULL to leave it out: a view weapon would float where
// the player stands, the frame before third person takes over
// the filter over the picture now: the shot's, or the editor's where it's shown
static int RC_FilterNow( void ) {
	if ( CE_Viewing() )
		return CM_EditorFilter();
	if ( rc.active && rc.filter && rc.shot.filterStrength > 0 )
		return rc.shot.filter;
	return RC_FI_NONE;
}

const refEntity_t *CL_RpCinematic_Entity( const refEntity_t *ent, refEntity_t *copy ) {
	if ( ( rc.active || CM_EditorOpen() ) && ( ent->renderfx & RF_FIRST_PERSON ) )
		return NULL;
	// the blades, for a cinematic's clashes; your own body left out while you act as someone else
	if ( CM_RecordHides( ent->origin ) )
		return NULL;
	if ( ent->reType == RT_SABER_GLOW )
		CM_NoteBlade( ent->origin, ent->axis[0], ent->saberLength );
	// a player or an NPC playing a shot's animation, on its frame
	if ( rcNumPlaying && ent->ghoul2 && ent->reType == RT_MODEL )
		RC_AnimateModel( ent );
	// one a cinematic tips over: its pitch and roll as well as its yaw, its size kept
	if ( ent->ghoul2 && ent->reType == RT_MODEL ) {
		vec3_t pos[8], angles[8];
		const int n = CL_NpcWalk_Tilts( pos, angles, ARRAY_LEN( pos ) );

		for ( int i = 0; i < n; i++ ) {
			float scale;

			if ( Distance( ent->origin, pos[i] ) > 48.0f )
				continue;
			scale = VectorLength( ent->axis[0] );
			*copy = *ent;
			AnglesToAxis( angles[i], copy->axis );
			for ( int j = 0; j < 3; j++ )
				VectorScale( copy->axis[j], scale > 0.0f ? scale : 1.0f, copy->axis[j] );
			ent = copy;
			break;
		}
	}
	// through a thermal camera, people are white hot
	if ( ent->ghoul2 && ent->reType == RT_MODEL && RC_FilterNow() == RC_FI_THERMAL ) {
		*copy = *ent;
		copy->customShader = cls.whiteShader;
		copy->renderfx &= ~RF_RGB_TINT;
		return copy;
	}
	return ent;
}

void CL_RpCinematic_Frame( void ) {
	RC_RunQueue();
	RC_RunAnims();
	CM_Frame();
}

// the black bars, over the scene and under the menus, then the editor
void CL_RpCinematic_Draw( void ) {
	static const vec4_t black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const float h = RC_BarsNow() * RC_BAR_FRAC * SCREEN_HEIGHT;

	if ( h >= 0.5f && !CE_Viewing() ) {
		re->SetColor( black );
		re->DrawStretchPic( 0, 0, SCREEN_WIDTH, h, 0, 0, 0, 0, cls.whiteShader );
		re->DrawStretchPic( 0, SCREEN_HEIGHT - h, SCREEN_WIDTH, h, 0, 0, 0, 0, cls.whiteShader );
		re->SetColor( NULL );
	}
	// the shot fading or wiping in or out, over the bars too
	if ( rc.active && rc.trans && !CE_Viewing() )
		RC_DrawTransitions( &rc.shot, cls.realtime - rc.start );
	if ( CL_RpCinematic_Subtitles() && !CE_Viewing() )
		CL_RpHooks_DrawSubtitles();
	CM_Draw();
}

/*
===============================================================================

Commands

===============================================================================
*/

// cincam here: the spot of the view as a hook writes it; cincam stop: leave the cinematic;
// cincam edit [shot]: the camera editor; cincam [local] play <shot>: a saved shot;
// cincam local <verb> [parts]: try it on this screen only; cincam <verb> [parts]: send it
static void CL_CinCam_f( void ) {
	const char *verb = Cmd_Argv( 1 ), *rest = Cmd_ArgsFrom( 2 );
	qboolean local = qfalse;
	char hook[MAX_STRING_CHARS];

	if ( !Q_stricmp( verb, "here" ) ) {
		vec3_t org, ang;
		float fovY;

		float fov;

		if ( CM_EditorCamera( org, ang, &fov ) ) {
			fovY = RC_FovY( fov );
		} else if ( rc.active ) {
			RC_Update();
			VectorCopy( rc.org, org );
			VectorCopy( rc.ang, ang );
			fovY = rc.fovY;
		} else if ( rc.haveView ) {
			VectorCopy( rc.viewOrg, org );
			VectorCopy( rc.viewAng, ang );
			fovY = rc.viewFovY;
		} else {
			Com_Printf( "Join a server first.\n" );
			return;
		}
		Com_Printf( "%i %i %i %i %i %i %i\n", RC_Round( org[0] ), RC_Round( org[1] ), RC_Round( org[2] ),
			RC_Round( AngleNormalize180( ang[PITCH] ) ), RC_Round( AngleNormalize360( ang[YAW] ) ),
			RC_Round( AngleNormalize180( ang[ROLL] ) ), RC_Round( RC_FovX( fovY, 4.0f, 3.0f ) ) );
		return;
	}
	if ( !Q_stricmp( verb, "stop" ) ) {
		CL_RpCinematic_Stop();
		return;
	}
	// an animation tried on this screen: cincam anim <who> <ANIMATION> [from] [to] [play|reverse|hold] [loop] [speed] [seconds]
	if ( !Q_stricmp( verb, "anim" ) ) {
		rcAnim_t a;

		if ( cls.state != CA_ACTIVE ) {
			Com_Printf( "Not connected to a server.\n" );
			return;
		}
		if ( !Q_stricmp( Cmd_Argv( 2 ), "stop" ) ) {
			RC_StopAnims( qfalse );
			return;
		}
		if ( Cmd_Argc() < 4 ) {
			Com_Printf( "cincam anim <me|number|name> <ANIMATION> [from] [to|-1] [play|reverse|hold] [loop|once] [speed %%] [seconds, 0 for good]\n"
				"cincam anim stop : let them all go\n" );
			return;
		}
		memset( &a, 0, sizeof( a ) );
		Q_strncpyz( a.who, Cmd_Argv( 2 ), sizeof( a.who ) );
		Q_strncpyz( a.anim, Cmd_Argv( 3 ), sizeof( a.anim ) );
		Q_strupr( a.anim );
		a.from = Cmd_Argc() > 4 ? atoi( Cmd_Argv( 4 ) ) : 0;
		a.to = Cmd_Argc() > 5 ? atoi( Cmd_Argv( 5 ) ) : -1;
		a.mode = !Q_stricmp( Cmd_Argv( 6 ), "reverse" ) ? RC_AM_REVERSE : !Q_stricmp( Cmd_Argv( 6 ), "hold" ) ? RC_AM_HOLD : RC_AM_PLAY;
		a.loop = (qboolean)!Q_stricmp( Cmd_Argv( 7 ), "loop" );
		a.speed = Cmd_Argc() > 8 ? Com_Clampi( 5, 1000, atoi( Cmd_Argv( 8 ) ) ) : 100;
		a.ms = Cmd_Argc() > 9 ? RC_SecondsMs( (float)atof( Cmd_Argv( 9 ) ) ) : 0;
		if ( !RC_FindAnim( RC_EntCfg( -1 ), a.anim ) )
			Com_Printf( S_COLOR_YELLOW "%s isn't one of the humanoid's animations; another model may have it\n", a.anim );
		RC_PlayAnims( &a, 1, 0, qtrue );
		return;
	}
	if ( !Q_stricmp( verb, "edit" ) ) {
		if ( CM_EditorOpen() && Cmd_Argc() < 3 )
			CL_RpCinematic_EditorClose();
		else
			CL_RpCinematic_Edit( Cmd_Argv( 2 ), qfalse );
		return;
	}
	if ( !Q_stricmp( verb, "local" ) ) {
		local = qtrue;
		verb = Cmd_Argv( 2 );
		rest = Cmd_ArgsFrom( 3 );
	}
	if ( !verb[0] ) {
		Com_Printf( "cincam edit [shot] : the camera editor, to make shots in\n"
			"cincam [local] play <shot> : a shot made there (local: on this screen only, to try it)\n"
			"cincam here : the spot you look from, as x y z pitch yaw roll fov\n"
			"cincam stop : leave the cinematic, on this screen\n"
			"cincam anim <who> <ANIMATION> [from] [to] [play|reverse|hold] [loop] [speed %] [seconds] : try one on this screen\n"
			"cincam [local] start [/hud] [/bars] : the cinematic starts\n"
			"cincam [local] path [/linear] [/look <x y z>] /<spot> [/<seconds> /<spot>...]\n"
			"cincam [local] move <seconds> [/linear] /<spot> [/<seconds> /<spot>...]\n"
			"cincam [local] cut <spot>\n"
			"cincam [local] orbit <seconds> [/linear] /<x y z> /<radius> /<height> /<from yaw> /<degrees> [/<fov> [/<end fov>]]\n"
			"cincam [local] lookat <x y z>|off\n"
			"cincam [local] fov <fov> [/<seconds>]\n"
			"cincam [local] bars on|off [/<seconds>]\n"
			"cincam [local] hud on|off\n"
			"cincam [local] end\n"
			"A spot is x y z [pitch yaw [roll [fov [focus range blur]]]]; parts are separated by /\n"
			"Before its spots a path or orbit can have /dof <focus|0 for what it sees|-1 for who> <range> <blur 1-10> [who],\n"
			"/filter <none|bw|noir|sepia|oldfilm|nightvision|thermal|cctv|vhs|hologram|warm|cold|faded|negative> <strength %>\n"
			"and /in|out <cut|fade|left|right|top|bottom> <seconds> <r g b>\n" );
		return;
	}
	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n" );
		return;
	}

	if ( !Q_stricmp( verb, "play" ) ) {
		const char *name = local ? Cmd_Argv( 3 ) : Cmd_Argv( 2 );

		if ( !CL_RpCinematic_ShotHook( name, hook, sizeof( hook ), NULL ) ) {
			Com_Printf( "No shot called \"%s\" in %s/\n", name, RC_SHOT_DIR );
			return;
		}
	} else {
		Com_sprintf( hook, sizeof( hook ), rest[0] ? "cin/%s/%s" : "cin/%s", verb, rest );
	}
	if ( local ) {
		RC_RunLocal( hook );
		if ( !Q_stricmp( verb, "play" ) )
			CL_RpCinematic_ShotWalks( Cmd_Argv( 3 ), qtrue );
		return;
	}
	if ( !CL_RpHooks_Experimental() ) {
		Com_Printf( S_COLOR_YELLOW "Not sent: " RP_EXPERIMENTAL_OFF " in the storyteller or NPC manager\n" );
		return;
	}
	// the server takes a // for a comment
	if ( strstr( hook, "//" ) ) {
		Com_Printf( S_COLOR_YELLOW "Not sent: an empty part would make a // the server cuts the line at\n" );
		return;
	}
	CL_AddReliableCommand( va( "rpnotify -1 1 cmd:%s", hook ), qfalse );
	if ( !Q_stricmp( verb, "play" ) )
		CL_RpCinematic_ShotWalks( Cmd_Argv( 2 ), qfalse );
}

void CL_RpCinematic_Init( void ) {
	memset( &rc, 0, sizeof( rc ) );
	RC_ForgetShots();
	rpc_cinematics = Cvar_Get( "rpc_cinematics", "1", CVAR_ARCHIVE_ND, "Let cinematics sent through RPMod take over the camera" );
	rpc_cinThirdPerson = Cvar_Get( "rpc_cinThirdPerson", "0", CVAR_ARCHIVE_ND | CVAR_INTERNAL, "" );
	// a cinematic was cut short by a crash: first person back
	RC_ThirdPerson( qfalse );
	Cmd_AddCommand( "cincam", CL_CinCam_f, "The cinematic camera: send shots by hand, or leave the one playing; cincam edit opens the cinematic manager" );
	CM_Init();
}

void CL_RpCinematic_Shutdown( void ) {
	Cmd_RemoveCommand( "cincam" );
	CM_Shutdown();
}

// leaving the map or the server ends it, bars, editor and all
void CL_RpCinematic_Reset( void ) {
	CL_RpCinematic_EditorClose();
	CM_Reset();
	RC_ResetAnims();
	rc.active = qfalse;
	rc.playing = qfalse;
	rc.bars = qfalse;
	rc.barsDuration = 0;
	rc.haveView = qfalse;
	RC_SyncThirdPerson();
}
