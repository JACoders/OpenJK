/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include "tr_local.h"

/*
=============================================================================

LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light
sources are visible.  The size of the flare relative to the screen is nearly
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
	struct		flare_s *next;		// for active chain

	int			addedFrame;
	uint32_t	testCount;

	portalView_t portalView;
	int			frameSceneNum;
	void		*surface;
	int			fogNum;

	int			fadeTime;

	qboolean	visible;			// state of last test
	float		drawIntensity;		// may be non 0 even if !visible due to fading

	int			windowX, windowY;
	float		eyeZ;
	float		drawZ;

	vec3_t		origin;
	vec3_t		color;
	vec3_t		normal;
} flare_t;

static flare_t	r_flareStructs[MAX_FLARES];
static flare_t *r_activeFlares, *r_inactiveFlares;

/*
==================
R_ClearFlares
==================
*/
void R_ClearFlares( void ) {
	int		i;

	if (!vk.fragmentStores)
		return;

	Com_Memset(r_flareStructs, 0, sizeof(r_flareStructs));
	r_activeFlares = NULL;
	r_inactiveFlares = NULL;

	for (i = 0; i < MAX_FLARES; i++) {
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[i];
	}
}

/*
==================
R_SearchFlare
==================
*/
static flare_t *R_SearchFlare( void *surface )
{
	flare_t *f;

	// see if a flare with a matching surface, scene, and view exists
	for ( f = r_activeFlares; f; f = f->next ) {
		if ( f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->portalView == backEnd.viewParms.portalView ) {
			return f;
		}
	}

	return NULL;
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

	if (normal && (normal[0] || normal[1] || normal[2])) {
		VectorSubtract(backEnd.viewParms.ori.origin, point, local);
		VectorNormalizeFast(local);
		d = fabs(DotProduct(local, normal));
		// If the viewer is behind the flare don't add it.
		//if (d < 0) {
		//	d = -d;
		//}
	}
	
	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip(point, backEnd.ori.modelViewMatrix, 
		backEnd.viewParms.projectionMatrix, eye, clip);

	// check to see if the point is completely off screen
	for (i = 0; i < 3; i++) {
		if (clip[i] >= clip[3] || clip[i] <= -clip[3]) {
			return;
		}
	}

	R_TransformClipToWindow(clip, &backEnd.viewParms, normalized, window);

	if (window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth 
		|| window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight) {
		return;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	f = R_SearchFlare(surface);

	// allocate a new one
	if (!f) {
		if (!r_inactiveFlares) {
			// the list is completely full
			return;
		}
		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next = r_activeFlares;
		r_activeFlares = f;

		f->surface = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->portalView = backEnd.viewParms.portalView;
		f->visible = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
		f->testCount = 0;
	}
	else {
		++f->testCount;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum = fogNum;

	VectorCopy(point, f->origin);
	VectorCopy(color, f->color);
	VectorCopy(normal, f->normal);

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	VectorScale(f->color, d , f->color);

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];

	f->eyeZ = eye[2];

#ifdef USE_REVERSED_DEPTH
	f->drawZ = (clip[2] + 0.10) / clip[3];
#else
	f->drawZ = (clip[2] - 0.10) / clip[3];
#endif

}

/*
==================
RB_AddDlightFlares
==================
*/
void RB_AddDlightFlares( void ) {
	dlight_t* l;
	int				i, j, k;
	fog_t* fog = NULL;

	if (!r_flares->integer) {
		return;
	}

	l = backEnd.refdef.dlights;

	if (tr.world)
		fog = tr.world->fogs;

	for (i = 0; i < backEnd.refdef.num_dlights; i++, l++) {

		if (fog)
		{
			// find which fog volume the light is in 
			for (j = 1; j < tr.world->numfogs; j++) {
				fog = &tr.world->fogs[j];
				for (k = 0; k < 3; k++) {
					if (l->origin[k] < fog->bounds[0][k] || l->origin[k] > fog->bounds[1][k]) {
						break;
					}
				}
				if (k == 3) {
					break;
				}
			}
			if (j == tr.world->numfogs) {
				j = 0;
			}
		}
		else
			j = 0;

		RB_AddFlare((void*)l, j, l->origin, l->color, NULL);
	}
}

/*
===============================================================================

FLARE BACK END

===============================================================================
*/

static float *vk_ortho(	float x1, float x2,
						float y2, float y1,
						float z1, float z2) 
{
	static float m[16] = { 0 };

	m[0] = 2.0f / (x2 - x1);
	m[5] = 2.0f / (y2 - y1);
	m[10] = 1.0f / (z1 - z2);
	m[12] = -(x2 + x1) / (x2 - x1);
	m[13] = -(y2 + y1) / (y2 - y1);
	m[14] = z1 / (z1 - z2);
	m[15] = 1.0f;

	return m;
}

/*
==================
RB_TestFlare
==================
*/
static void RB_TestFlare( flare_t *f ) {
	qboolean		visible;
	float			fade;
	float			*m;
	uint32_t		offset;
	int				i;

	backEnd.pc.c_flareTests++;

	/*
		We don't have equivalent of glReadPixels() in vulkan
		and explicit depth buffer reading may be very slow and require surface conversion.

		So we will use storage buffer and exploit early depth tests by
		rendering test dot in orthographic projection at projected flare coordinates
		window-x, window-y and world-z: if test dot is not covered by
		any world geometry - it will invoke fragment shader which will
		fill storage buffer at desired location, then we discard fragment.
		In next frame we read storage buffer: if there is a non-zero value
		then our flare WAS visible (as we're working with 1-frame delay),
		multisampled image will cause multiple fragment shader invocations.
	*/

	// we neeed only single uint32_t but take care of alignment
	offset = (f - r_flareStructs) * vk.storage_alignment;

	if (f->testCount) {
		uint32_t *cnt = (uint32_t*)(vk.storage.buffer_ptr + offset);
		if (*cnt)
			visible = qtrue;
		else
			visible = qfalse;

		f->testCount = 1;
	}
	else {
		visible = qfalse;
	}

	// reset test result in storage buffer
	//Com_Memset(vk.storage.buffer_ptr + offset, 0x0, sizeof(uint32_t));
	// *((uint32_t*)(vk.storage.buffer_ptr + offset)) = 0x00;

	m = vk_ortho(backEnd.viewParms.viewportX, backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
		backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, 0, 1);

	vk_update_mvp(m);

	tess.xyz[0][0] = f->windowX;
	tess.xyz[0][1] = f->windowY;
	tess.xyz[0][2] = -f->drawZ;
	tess.numVertexes = 1;

#ifdef USE_VBO
	tess.vbo_world_index = 0;
#endif
	// invalidate descriptors
	for ( i = 0; i < VK_DESC_COUNT; i++ ) {
		vk_reset_descriptor( i );
	}
	// render test dot
	vk_bind_pipeline( vk.std_pipeline.dot_pipeline );
	vk_bind_geometry( TESS_XYZ );
	vk_draw_dot( offset );

	if (visible) {
		if (!f->visible) {
			f->visible = qtrue;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		//fade = ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;
		fade = ( ( backEnd.refdef.time - f->fadeTime ) / 500.0f );
	}
	else {
		// Dont fade out when flare is occluded. Will result in the ability to see
		// flares through surfaces on high movement speeds
		/*if (f->visible) {
			f->visible = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;*/
		fade = 0.0f;
	}

	if (fade < 0) {
		fade = 0;
	}
	else if (fade > 1) {
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
	color4ub_t		color;

	backEnd.pc.c_flareRenders++;

	srfFlare_t *flare = (srfFlare_t *)f->surface;

	backEnd.currentEntity = &tr.worldEntity;
	RB_BeginSurface( flare->shader, f->fogNum );

	vec3_t		dir;
	vec3_t		left, up;
	vec3_t		origin;
	float		d, dist;

	// calculate the xyz locations for the four corners
	VectorMA( f->origin, 3, f->normal, origin );
	float* snormal = f->normal;

	VectorSubtract( origin, backEnd.viewParms.ori.origin, dir );
	dist = VectorNormalize( dir );

	d = -DotProduct( dir, snormal );
	if ( d < 0 )
		d = -d;

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	color[0] = d * 255.0f;
	color[1] = d * 255.0f;
	color[2] = d * 255.0f;
	color[3] = 255.0f;	//only gets used if the shader has cgen exact_vertex!
	
	float radius = tess.shader->portalRange ? tess.shader->portalRange : 30;
	if ( dist < 512.0f )
		radius = radius * dist / 512.0f;

	if ( radius < 5.0f )
		radius = 5.0f;

	VectorScale( backEnd.viewParms.ori.axis[1], radius, left );
	VectorScale( backEnd.viewParms.ori.axis[2], radius, up );
	if ( backEnd.viewParms.portalView == PV_MIRROR ) {
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( origin, left, up, color );

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
void RB_RenderFlares( void ) {
	flare_t* f;
	flare_t** prev;
	qboolean	draw;

	if ( !r_flares->integer )
		return;

	if ( vk.renderPassIndex == RENDER_PASS_SCREENMAP )
		return;

	if ( backEnd.isHyperspace )
		return;

	// Reset currentEntity to world so that any previously referenced entities
	// don't have influence on the rendering of these flares (i.e. RF_ renderer flags).
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.ori = backEnd.viewParms.world;

	//RB_AddDlightFlares();

	// perform z buffer readback on each flare in this view
	draw = qfalse;
	prev = &r_activeFlares;
	while ( ( f = *prev ) != NULL ) {
		// throw out any flares that weren't added last frame
		if ( backEnd.viewParms.frameCount - f->addedFrame > 0 && f->portalView == backEnd.viewParms.portalView ) {
			*prev = f->next;
			f->next = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->portalView == backEnd.viewParms.portalView ) {
			RB_TestFlare( f );
			if ( f->testCount == 0 ) {
				// recently added, wait 1 frame for test result
			}
			else if ( f->drawIntensity ) {
				draw = qtrue;
			}
			else {
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

	vk_update_mvp( NULL );

	for ( f = r_activeFlares; f; f = f->next ) {
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->drawIntensity && f->portalView == backEnd.viewParms.portalView ) {
			RB_RenderFlare( f );
		}
	}
}