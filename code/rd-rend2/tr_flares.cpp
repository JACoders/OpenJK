/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_flares.c

#include "tr_local.h"

/*
=============================================================================

LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light
sources are visible.  The size of the flare reletive to the screen is nearly
constant, irrespective of distance, but the intensity should be proportional to the
projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that its midpoint should have when the surface is added.

After all opaque surfaces have been rendered, the depth buffer is read back for
each flare in view.  If the point has not been obscured by a closer surface, the
flare should be drawn.

Surfaces that have a repeated texture should never be flagged as flaring, because
there will only be a single flare added at the midpoint of the polygon.

To prevent abrupt popping, the intensity of the flare is interpolated up and
down as it changes visibility.  This involves scene to scene state, unlike almost
all other aspects of the renderer, and is complicated by the fact that a single
frame may have multiple scenes.

RB_RenderFlares() will be called once per view (twice in a mirrored scene, potentially
up to five or more times in a frame with 3D status bar icons).

=============================================================================
*/


// flare states maintain visibility over multiple frames for fading
// layers: view, mirror, menu
typedef struct flare_s {
	struct		flare_s	*next;		// for active chain

	int			addedFrame;

	qboolean	inPortal;				// true if in a portal view of the scene
	int			frameSceneNum;
	void		*surface;
	int			fogNum;

	int			fadeTime;

	qboolean	visible;			// state of last test
	float		drawIntensity;		// may be non 0 even if !visible due to fading

	int			windowX, windowY;
	float		eyeZ;

	vec3_t		origin;
	vec3_t		color;
	vec3_t		normal;
} flare_t;

#define		MAX_FLARES		128

flare_t		r_flareStructs[MAX_FLARES];
flare_t		*r_activeFlares, *r_inactiveFlares;

/*
==================
R_ClearFlares
==================
*/
void R_ClearFlares( void ) {
	int		i;

	Com_Memset( r_flareStructs, 0, sizeof( r_flareStructs ) );
	r_activeFlares = NULL;
	r_inactiveFlares = NULL;

	for ( i = 0 ; i < MAX_FLARES ; i++ ) {
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[i];
	}
}


/*
==================
RB_AddFlare

This is called at surface tesselation time
==================
*/
void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal ) {
	int				i;
	flare_t			*f;
	vec3_t			local;
	float			d = 1.0f;
	vec4_t			eye, clip, normalized, window;

	backEnd.pc.c_flareAdds++;

	if(normal && (normal[0] || normal[1] || normal[2]))
	{
		VectorSubtract( backEnd.viewParms.ori.origin, point, local );
		VectorNormalizeFast(local);
		d = DotProduct(local, normal);
	}

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip( point, backEnd.ori.modelViewMatrix,
		backEnd.viewParms.projectionMatrix, eye, clip );

	// check to see if the point is completely off screen
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( clip[i] >= clip[3] || clip[i] <= -clip[3] ) {
			return;
		}
	}

	R_TransformClipToWindow( clip, &backEnd.viewParms, normalized, window );

	if ( window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth
		|| window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight ) {
		return;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	for ( f = r_activeFlares ; f ; f = f->next ) {
		if ( f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum
			&& f->inPortal == backEnd.viewParms.isPortal ) {
			break;
		}
	}

	// allocate a new one
	if (!f ) {
		if ( !r_inactiveFlares ) {
			// the list is completely full
			return;
		}
		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next = r_activeFlares;
		r_activeFlares = f;

		f->surface = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal = backEnd.viewParms.isPortal;
		f->addedFrame = -1;
	}

	if ( f->addedFrame != backEnd.viewParms.frameCount - 1 ) {
		f->visible = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum = fogNum;

	VectorCopy(point, f->origin);
	VectorCopy( color, f->color );
	VectorCopy( normal, f->normal);

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	VectorScale( f->color, d, f->color ); 

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];

	f->eyeZ = eye[2];
}

/*
==================
RB_AddDlightFlares
==================
*/
void RB_AddDlightFlares( void ) {
	dlight_t		*l;
	int				i, j, k;
	fog_t			*fog = NULL;

	if ( !r_flares->integer ) {
		return;
	}

	l = backEnd.refdef.dlights;

	if(tr.world)
		fog = tr.world->fogs;

	for (i=0 ; i<backEnd.refdef.num_dlights ; i++, l++) {

		if(fog)
		{
			// find which fog volume the light is in 
			for ( j = 1 ; j < tr.world->numfogs ; j++ ) {
				fog = &tr.world->fogs[j];
				for ( k = 0 ; k < 3 ; k++ ) {
					if ( l->origin[k] < fog->bounds[0][k] || l->origin[k] > fog->bounds[1][k] ) {
						break;
					}
				}
				if ( k == 3 ) {
					break;
				}
			}
			if ( j == tr.world->numfogs ) {
				j = 0;
			}
		}
		else
			j = 0;

		RB_AddFlare( (void *)l, j, l->origin, l->color, NULL );
	}
}

/*
===============================================================================

FLARE BACK END

===============================================================================
*/

/*
==================
RB_TestFlare
==================
*/
void RB_TestFlare( flare_t *f ) {
	float			depth;
	qboolean		visible;
	float			fade;
	float			screenZ;
	FBO_t           *oldFbo;

	backEnd.pc.c_flareTests++;

	// if we're doing multisample rendering, read from the correct FBO
	oldFbo = glState.currentFBO;
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(tr.msaaResolveFbo);
	}

	// read back the z buffer contents, which is bad
	// TODO: Don't use glReadPixels
	qglReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );

	// if we're doing multisample rendering, switch to the old FBO
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(oldFbo);
	}

	screenZ = backEnd.viewParms.projectionMatrix[14] / 
		( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

	visible = (qboolean)(( -f->eyeZ - -screenZ ) < 24);

	if ( visible ) {
		if ( !f->visible ) {
			f->visible = qtrue;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = ( ( backEnd.refdef.time - f->fadeTime ) / 500.0f );
	} else {
		// Dont fade out when flare is occluded. Will result in the ability to see 
		// flares through surfaces on high movement speeds
		/*if ( f->visible ) {
			f->visible = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;*/
		fade = 0.0f;
	}

	if ( fade < 0 ) {
		fade = 0;
	}
	if ( fade > 1 ) {
		fade = 1;
	}

	f->drawIntensity = fade;
}


/*
==================
RB_RenderFlare
==================
*/
void RB_RenderFlare( flare_t *f ) {
	vec4_t			color;

	backEnd.pc.c_flareRenders++;

	srfFlare_t *flare = (srfFlare_t *)f->surface;

	backEnd.currentEntity = &tr.worldEntity;
	RB_BeginSurface( flare->shader, f->fogNum, 0 );

	vec3_t		dir;
	vec3_t		left, up;
	vec3_t		origin;
	float		d, dist;

	// calculate the xyz locations for the four corners
	VectorMA(f->origin, 3, f->normal, origin);
	float* snormal = f->normal;

	VectorSubtract(origin, backEnd.viewParms.ori.origin, dir);
	dist = VectorNormalize(dir);

	d = -DotProduct(dir, snormal);
	if (d < 0) {
		d = -d;
	}

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	color[0] = d;
	color[1] = d;
	color[2] = d;
	color[3] = 1.0f;	//only gets used if the shader has cgen exact_vertex!

	float radius = tess.shader->portalRange ? tess.shader->portalRange : 30;
	if (dist < 512.0f)
	{
		radius = radius * dist / 512.0f;
	}
	if (radius < 5.0f)
	{
		radius = 5.0f;
	}
	VectorScale(backEnd.viewParms.ori.axis[1], radius, left);
	VectorScale(backEnd.viewParms.ori.axis[2], radius, up);
	if (backEnd.viewParms.isMirror) {
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(origin, left, up, color);

	RB_EndSurface();
}

/*
==================
RB_RenderFlares

Because flares are simulating an occular effect, they should be drawn after
everything (all views) in the entire frame has been drawn.

Because of the way portals use the depth buffer to mark off areas, the
needed information would be lost after each view, so we are forced to draw
flares after each view.

The resulting artifact is that flares in mirrors or portals don't dim properly
when occluded by something in the main view, and portal flares that should
extend past the portal edge will be overwritten.
==================
*/
void RB_RenderFlares (void) {
	flare_t		*f;
	flare_t		**prev;
	qboolean	draw;

	if ( !r_flares->integer ) {
		return;
	}

	if (backEnd.viewParms.flags & VPF_DEPTHSHADOW)
		return;

	// Reset currentEntity to world so that any previously referenced entities
	// don't have influence on the rendering of these flares (i.e. RF_ renderer flags).
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.ori = backEnd.viewParms.world;

//	RB_AddDlightFlares();

	// perform z buffer readback on each flare in this view
	draw = qfalse;
	prev = &r_activeFlares;
	while ( ( f = *prev ) != NULL ) {
		// throw out any flares that weren't added last frame
		if ( f->addedFrame < backEnd.viewParms.frameCount - 1 ) {
			*prev = f->next;
			f->next = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum
			&& f->inPortal == backEnd.viewParms.isPortal ) {
			RB_TestFlare( f );
			if ( f->drawIntensity ) {
				draw = qtrue;
			} else {
				// this flare has completely faded out, so remove it from the chain
				*prev = f->next;
				f->next = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		prev = &f->next;
	}

	if ( !draw ) {
		return;		// none visible
	}

	for ( f = r_activeFlares ; f ; f = f->next ) {
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum
			&& f->inPortal == backEnd.viewParms.isPortal
			&& f->drawIntensity ) {
			RB_RenderFlare( f );
		}
	}
}





