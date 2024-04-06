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

#include "tr_local.h"

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/


// copied and adapted from tr_mesh.c

/*
=============
R_MDRCullModel
=============
*/

static int R_MDRCullModel( mdrHeader_t *header, trRefEntity_t *ent ) {
	vec3_t		bounds[2];
	mdrFrame_t	*oldFrame, *newFrame;
	int			i, frameSize;

	frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	// compute frame pointers
	newFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.frame);
	oldFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.oldframe);

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		if ( ent->e.frame == ent->e.oldframe )
		{
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
				// Ummm... yeah yeah I know we don't really have an md3 here.. but we pretend
				// we do. After all, the purpose of mdrs are not that different, are they?

				case CULL_OUT:
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;

				case CULL_IN:
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;

				case CULL_CLIP:
					tr.pc.c_sphere_cull_md3_clip++;
					break;
			}
		}
		else
		{
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB )
			{
				if ( sphereCull == CULL_OUT )
				{
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				}
				else if ( sphereCull == CULL_IN )
				{
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				}
				else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	for (i = 0 ; i < 3 ; i++) {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
		case CULL_IN:
			tr.pc.c_box_cull_md3_in++;
			return CULL_IN;
		case CULL_CLIP:
			tr.pc.c_box_cull_md3_clip++;
			return CULL_CLIP;
		case CULL_OUT:
		default:
			tr.pc.c_box_cull_md3_out++;
			return CULL_OUT;
	}
}

/*
=================
R_MDRComputeFogNum

=================
*/

int R_MDRComputeFogNum( mdrHeader_t *header, trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;
	mdrFrame_t		*mdrFrame;
	vec3_t			localOrigin;
	int frameSize;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	// FIXME: non-normalized axis issues
	mdrFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.frame);
	VectorAdd( ent->e.origin, mdrFrame->localOrigin, localOrigin );
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( localOrigin[j] - mdrFrame->radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( localOrigin[j] + mdrFrame->radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}


/*
==============
R_MDRAddAnimSurfaces
==============
*/

// much stuff in there is just copied from R_AddMd3Surfaces in tr_mesh.c

void R_MDRAddAnimSurfaces( trRefEntity_t *ent, int entityNum ) {
	mdrHeader_t		*header;
	mdrSurface_t	*surface;
	mdrLOD_t		*lod;
	shader_t		*shader;
	skin_t		*skin;
	int				i, j;
	int				lodnum = 0;
	int				fogNum = 0;
	int				cull;
	int             cubemapIndex;
	qboolean	personalModel;

	header = (mdrHeader_t *)tr.currentModel->data.mdr;

	personalModel = (qboolean)(
		(ent->e.renderfx & RF_THIRD_PERSON) &&
		!(tr.viewParms.isPortal ||
			(tr.viewParms.flags & VPF_DEPTHSHADOW)));

	if ( ent->e.renderfx & RF_WRAP_FRAMES )
	{
		ent->e.frame %= header->numFrames;
		ent->e.oldframe %= header->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ((ent->e.frame >= header->numFrames)
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= header->numFrames)
		|| (ent->e.oldframe < 0) )
	{
		ri.Printf( PRINT_DEVELOPER, "R_MDRAddAnimSurfaces: no such frame %d to %d for '%s'\n",
			   ent->e.oldframe, ent->e.frame, tr.currentModel->name );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_MDRCullModel (header, ent);
	if ( cull == CULL_OUT ) {
		return;
	}

	// figure out the current LOD of the model we're rendering, and set the lod pointer respectively.
	lodnum = R_ComputeLOD(ent);
	// check whether this model has as that many LODs at all. If not, try the closest thing we got.
	if(header->numLODs <= 0)
		return;
	if(header->numLODs <= lodnum)
		lodnum = header->numLODs - 1;

	lod = (mdrLOD_t *)( (byte *)header + header->ofsLODs);
	for(i = 0; i < lodnum; i++)
	{
		lod = (mdrLOD_t *) ((byte *) lod + lod->ofsEnd);
	}

	// fogNum?
	fogNum = R_MDRComputeFogNum( header, ent );

	cubemapIndex = R_CubemapForPoint(ent->e.origin);

	surface = (mdrSurface_t *)( (byte *)lod + lod->ofsSurfaces );

	for ( i = 0 ; i < lod->numSurfaces ; i++ )
	{

		if(ent->e.customShader)
			shader = R_GetShaderByHandle(ent->e.customShader);
		else if(ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
		{
			skin = R_GetSkinByHandle(ent->e.customSkin);
			shader = tr.defaultShader;

			for(j = 0; j < skin->numSurfaces; j++)
			{
				if (!strcmp(skin->surfaces[j]->name, surface->name))
				{
					shader = (shader_t *)skin->surfaces[j]->shader;
					break;
				}
			}
		}
		else if(surface->shaderIndex > 0)
			shader = R_GetShaderByHandle( surface->shaderIndex );
		else
			shader = tr.defaultShader;

		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
		        && r_shadows->integer == 2
			&& fogNum == 0
			&& !(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			&& shader->sort == SS_OPAQUE )
		{
			R_AddDrawSurf( (surfaceType_t *)surface, entityNum, tr.shadowShader, 0, qfalse, R_IsPostRenderEntity(ent), 0 );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			&& fogNum == 0
			&& (ent->e.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE )
		{
			R_AddDrawSurf( (surfaceType_t *)surface, entityNum, tr.projectionShadowShader, 0, qfalse, R_IsPostRenderEntity(ent), 0 );
		}

		if (!personalModel)
			R_AddDrawSurf( (surfaceType_t *)surface, entityNum, shader, fogNum, qfalse, R_IsPostRenderEntity(ent), cubemapIndex );

		surface = (mdrSurface_t *)( (byte *)surface + surface->ofsEnd );
	}
}

/*
==============
RB_MDRSurfaceAnim
==============
*/
void RB_MDRSurfaceAnim( mdrSurface_t *surface )
{
	int				i, j, k;
	float			frontlerp, backlerp;
	int				*triangles;
	int				indexes;
	int				baseIndex, baseVertex;
	int				numVerts;
	mdrVertex_t		*v;
	mdrHeader_t		*header;
	mdrFrame_t		*frame;
	mdrFrame_t		*oldFrame;
	mdrBone_t		bones[MDR_MAX_BONES], *bonePtr, *bone;

	int			frameSize;

	// don't lerp if lerping off, or this is the only frame, or the last frame...
	//
	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame)
	{
		backlerp	= 0;	// if backlerp is 0, lerping is off and frontlerp is never used
		frontlerp	= 1;
	}
	else
	{
		backlerp	= backEnd.currentEntity->e.backlerp;
		frontlerp	= 1.0f - backlerp;
	}

	header = (mdrHeader_t *)((byte *)surface + surface->ofsHeader);

	frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	frame = (mdrFrame_t *)((byte *)header + header->ofsFrames +
		backEnd.currentEntity->e.frame * frameSize );
	oldFrame = (mdrFrame_t *)((byte *)header + header->ofsFrames +
		backEnd.currentEntity->e.oldframe * frameSize );

	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	triangles	= (int *) ((byte *)surface + surface->ofsTriangles);
	indexes		= surface->numTriangles * 3;
	baseIndex	= tess.numIndexes;
	baseVertex	= tess.numVertexes;

	// Set up all triangles.
	for (j = 0 ; j < indexes ; j++)
	{
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !backlerp )
	{
		// no lerping needed
		bonePtr = frame->bones;
	}
	else
	{
		bonePtr = bones;

		for ( i = 0 ; i < header->numBones*12 ; i++ )
		{
			((float *)bonePtr)[i] = frontlerp * ((float *)frame->bones)[i] + backlerp * ((float *)oldFrame->bones)[i];
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	v = (mdrVertex_t *) ((byte *)surface + surface->ofsVerts);
	for ( j = 0; j < numVerts; j++ )
	{
		vec3_t	tempVert, tempNormal;
		mdrWeight_t	*w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ )
		{
			bone = bonePtr + w->boneIndex;

			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );

			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j] = R_VboPackNormal(tempNormal);

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (mdrVertex_t *)&v->weights[v->numWeights];
	}

	tess.numVertexes += surface->numVerts;
}
