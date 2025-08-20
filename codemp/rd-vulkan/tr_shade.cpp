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

// tr_shade.c

#include "tr_local.h"
#include "tr_quicksprite.h"

shaderCommands_t	tess;
color4ub_t			styleColors[MAX_LIGHT_STYLES];

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

#ifdef USE_VBO
	if (shader->isStaticShader && !shader->remappedShader) {
		tess.allowVBO = qtrue;
	}
	else {
		tess.allowVBO = qfalse;
	}
#endif

#ifdef USE_PMLIGHT
	if (tess.fogNum != fogNum) {
		tess.dlightUpdateParams = qtrue;
	}
#endif

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.multiDrawPrimitives = 0;

	tess.shader = state;
	tess.fogNum = fogNum;
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}

	tess.fading = false;

	tess.registration++;
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	const shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0) {
		return;
	}

	if (input->numIndexes > SHADER_MAX_INDEXES) {
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}

	if (input->numVertexes > SHADER_MAX_VERTEXES) {
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort && !backEnd.doneSurfaces) {
#ifdef USE_VBO
		tess.vbo_world_index = 0; //VBO_UnBind();
#endif
		return;
	}

	if ( skyboxportal ) {
		// world
		if( !( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) ) {
			// don't process these tris at all
			if( tess.shader->optimalStageIteratorFunc == RB_StageIteratorSky )	
				return;
		}

		// portal sky
		else if( !drawskyboxportal ) {
			// /only/ process sky tris
			if( !( tess.shader->optimalStageIteratorFunc == RB_StageIteratorSky ) )
				return;
		}
	}

	//
	// update performance counters
	//
#ifdef USE_PMLIGHT
	if (tess.dlightPass) {
		backEnd.pc.c_lit_batches++;
		backEnd.pc.c_lit_vertices += tess.numVertexes;
		backEnd.pc.c_lit_indices += tess.numIndexes;
	}
	else
#endif
	{
		backEnd.pc.c_shaders++;
		backEnd.pc.c_vertexes += tess.numVertexes;
		backEnd.pc.c_indexes += tess.numIndexes;
	}
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	// Fogpass
	//if (tess.fogNum && tess.shader->fogPass && r_drawfog->value == 1)
	//{
	//	backEnd.pc.c_totalIndexes += tess.numIndexes;
	//}

	//
	// call off to shader specific tess end function
	//
	tess.shader->optimalStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris (input);
	}
	if ( r_shownormals->integer ) {
		DrawNormals(input);
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.multiDrawPrimitives = 0;

#ifdef USE_VBO
	tess.vbo_world_index = 0;
	tess.vbo_model = nullptr;
	tess.ibo_model = nullptr;
	//VBO_ClearQueue();
#endif
}

