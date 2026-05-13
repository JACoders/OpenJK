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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"

/*
  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]
*/

typedef struct {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES / 3];

static void R_AddEdgeDef(int i1, int i2, int facing) {
	int		c = numEdgeDefs[i1];
	if (c == MAX_EDGE_DEFS) {
		return;		// overflow
	}
	edgeDefs[i1][c].i2 = i2;
	edgeDefs[i1][c].facing = facing;

	numEdgeDefs[i1]++;
}

static void R_CalcShadowEdges(void) {
	qboolean	sil_edge;
	int			i;
	int			c, c2;
	int			j, k;
	int			i2;
	color4ub_t *colors;

	tess.numIndexes = 0;

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	for (i = 0; i < tess.numVertexes; i++) {
		c = numEdgeDefs[i];
		for (j = 0; j < c; j++) {
			if (!edgeDefs[i][j].facing) {
				continue;
			}

			sil_edge = qtrue;
			i2 = edgeDefs[i][j].i2;
			c2 = numEdgeDefs[i2];
			for (k = 0; k < c2; k++) {
				if (edgeDefs[i2][k].i2 == i && edgeDefs[i2][k].facing) {
					sil_edge = qfalse;
					break;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if (sil_edge) {
				if (tess.numIndexes > ARRAY_LEN(tess.indexes) - 6) {
					i = tess.numVertexes;
					break;
				}

				tess.indexes[tess.numIndexes + 0] = i;
				tess.indexes[tess.numIndexes + 1] = i2;
				tess.indexes[tess.numIndexes + 2] = i + tess.numVertexes;
				tess.indexes[tess.numIndexes + 3] = i2;
				tess.indexes[tess.numIndexes + 4] = i2 + tess.numVertexes;
				tess.indexes[tess.numIndexes + 5] = i + tess.numVertexes;

				tess.numIndexes += 6;
			}
		}
	}

	tess.numVertexes *= 2;

	colors = &tess.svars.colors[0][0]; // we need at least 2x SHADER_MAX_VERTEXES there

	for (i = 0; i < tess.numVertexes; i++) {
		Vector4Set(colors[i], 50, 50, 50, 255);
	}
}

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd(void) {
	int			i, numTris;
	vec3_t		lightDir;
	uint32_t	pipeline[2];

	if (glConfig.stencilBits < 4)
		return;

#if 1
	vec3_t	entLight;
	vec3_t	worldxyz;
	float	groundDist;

#ifdef USE_PMLIGHT
	if (r_dlightMode->integer == 2 && r_shadows->integer == 2)
		VectorCopy(backEnd.currentEntity->shadowLightDir, entLight);
	else
#endif
		VectorCopy(backEnd.currentEntity->modelLightDir, entLight);

	entLight[2] = 0.0f;
	VectorNormalize(entLight);

	//Oh well, just cast them straight down no matter what onto the ground plane.
	//This presets no chance of screwups and still looks better than a stupid
	//shader blob.
	VectorSet(lightDir, entLight[0] * 0.3f, entLight[1] * 0.3f, 1.0f);
	
	// project vertexes away from light direction
	for (i = 0; i < tess.numVertexes; i++) {
		//add or.origin to vert xyz to end up with world oriented coord, then figure
		//out the ground pos for the vert to project the shadow volume to
		VectorAdd(tess.xyz[i], backEnd.ori.origin, worldxyz);
		groundDist = worldxyz[2] - backEnd.currentEntity->e.shadowPlane;
		groundDist += 16.0f; //fudge factor
		VectorMA(tess.xyz[i], -groundDist, lightDir, tess.xyz[i + tess.numVertexes]);
	}
#else
#ifdef USE_PMLIGHT
	if (r_dlightMode->integer == 2 && r_shadows->integer == 2)
		VectorCopy(backEnd.currentEntity->shadowLightDir, lightDir);
	else
#endif
		VectorCopy(backEnd.currentEntity->modelLightDir, lightDir);

	// clamp projection by height
	if (lightDir[2] > 0.1) {
		float s = 0.1 / lightDir[2];
		VectorScale(lightDir, s, lightDir);
	}

	// project vertexes away from light direction
	for (i = 0; i < tess.numVertexes; i++) {
		VectorMA(tess.xyz[i], -512, lightDir, tess.xyz[i + tess.numVertexes]);
	}
#endif 

	// decide which triangles face the light
	Com_Memset(numEdgeDefs, 0, tess.numVertexes * sizeof(numEdgeDefs[0]));

	numTris = tess.numIndexes / 3;
	for (i = 0; i < numTris; i++)
	{
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1];
		i3 = tess.indexes[ i*3 + 2];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		d = DotProduct( normal, lightDir );
		if ( d > 0 ) {
			facing[i] = 1;
		}
		else {
			facing[i] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[i] );
		R_AddEdgeDef( i2, i3, facing[i] );
		R_AddEdgeDef( i3, i1, facing[i] );
	}
	
	R_CalcShadowEdges();

	// draw the silhouette edges
	vk_bind(tr.whiteImage);

	// mirrors have the culling order reversed
	if (backEnd.viewParms.portalView == PV_MIRROR) {
		pipeline[0] = vk.std_pipeline.shadow_volume_pipelines[0][1];
		pipeline[1] = vk.std_pipeline.shadow_volume_pipelines[1][1];
	}
	else {
		pipeline[0] = vk.std_pipeline.shadow_volume_pipelines[0][0];
		pipeline[1] = vk.std_pipeline.shadow_volume_pipelines[1][0];
	}

	vk_bind_pipeline(pipeline[0]); // back-sided
	vk_bind_index();
	vk_bind_geometry(TESS_XYZ | TESS_RGBA0);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);
	vk_bind_pipeline(pipeline[1]); // front-sided
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);

	tess.numVertexes /= 2;

	backEnd.doneShadows = qtrue;
	tess.numIndexes = 0;
}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish(void)
{
	float tmp[16];
	int i;

	if (!backEnd.doneShadows)
		return;

	backEnd.doneShadows = qfalse;

	if (r_shadows->integer != 2)
		return;
	
	if (glConfig.stencilBits < 4)
		return;

	static const vec3_t verts[4] = {
		{ -100, 100, -10 },
		{  100, 100, -10 },
		{ -100,-100, -10 },
		{  100,-100, -10 }
	};

	vk_bind(tr.whiteImage);

	for (i = 0; i < 4; i++)
	{
		VectorCopy(verts[i], tess.xyz[i]);
		Vector4Set(tess.svars.colors[0][i], 153, 153, 153, 255);
	}
	tess.numVertexes = 4;

	Com_Memcpy(tmp, vk_world.modelview_transform, 64);
	Com_Memset(vk_world.modelview_transform, 0, 64);

	vk_world.modelview_transform[0] = 1.0f;
	vk_world.modelview_transform[5] = 1.0f;
	vk_world.modelview_transform[10] = 1.0f;
	vk_world.modelview_transform[15] = 1.0f;

	vk_bind_pipeline(vk.std_pipeline.shadow_finish_pipeline);
	vk_update_mvp(NULL);
	vk_bind_geometry(TESS_XYZ | TESS_RGBA0);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qfalse);

	Com_Memcpy(vk_world.modelview_transform, tmp, 64);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform(void)
{
	int		i;
	float	*xyz;
	vec3_t	ground;
	float	groundDist, d, h;
	vec3_t	light;
	vec3_t	lightDir;

	xyz = (float*)tess.xyz;

	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	groundDist = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

#ifdef USE_PMLIGHT
	if (r_dlightMode->integer == 2 && r_shadows->integer == 2)
		VectorCopy(backEnd.currentEntity->shadowLightDir, lightDir);
	else
#endif
		VectorCopy(backEnd.currentEntity->modelLightDir, lightDir);

	d = DotProduct(lightDir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5)
	{
		VectorMA(lightDir, (0.5 - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;


	for (i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		h = DotProduct(xyz, ground) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}