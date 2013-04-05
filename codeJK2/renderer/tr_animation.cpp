// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


#include "tr_local.h"
#include "MatComp.h"
extern int R_ComputeLOD( trRefEntity_t *ent );

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the 
orientation of the bone in the base frame to the orientation in this
frame.

*/


/*
=============
R_ACullModel
=============
*/
static int R_ACullModel( md4Header_t *header, trRefEntity_t *ent ) {
	vec3_t		bounds[2];
	md4Frame_t	*oldFrame, *newFrame;
	int			i;
	int			frameSize;
	// compute frame pointers

	if (header->ofsFrames<0) // Compressed
	{
		frameSize = (int)( &((md4CompFrame_t *)0)->bones[ tr.currentModel->md4->numBones ] );		
		newFrame = (md4Frame_t *)((byte *)header - header->ofsFrames + ent->e.frame * frameSize );
		oldFrame = (md4Frame_t *)((byte *)header - header->ofsFrames + ent->e.oldframe * frameSize );
		// HACK! These frames actually are md4CompFrames, but the first fields are the same, 
		// so this will work for this routine.
	}
	else
	{
		frameSize = (int)( &((md4Frame_t *)0)->bones[ tr.currentModel->md4->numBones ] );		
		newFrame = (md4Frame_t *)((byte *)header + header->ofsFrames + ent->e.frame * frameSize );
		oldFrame = (md4Frame_t *)((byte *)header + header->ofsFrames + ent->e.oldframe * frameSize );
	}

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		if ( ent->e.frame == ent->e.oldframe )
		{
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
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
R_AComputeFogNum

=================
*/
static int R_AComputeFogNum( md4Header_t *header, trRefEntity_t *ent ) {
	int				i;
	fog_t			*fog;
	md4Frame_t		*frame;
	vec3_t			localOrigin;
	int				frameSize;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}


	if (header->ofsFrames<0) // Compressed
	{
		frameSize = (int)( &((md4CompFrame_t *)0)->bones[ header->numBones ] );		
		frame = (md4Frame_t *)((byte *)header - header->ofsFrames + ent->e.frame * frameSize );
		// HACK! These frames actually are md4CompFrames, but the first fields are the same, 
		// so this will work for this routine.
	}
	else
	{
		frameSize = (int)( &((md4Frame_t *)0)->bones[ header->numBones ] );		
		frame = (md4Frame_t *)((byte *)header + header->ofsFrames + ent->e.frame * frameSize );
	}

	VectorAdd( ent->e.origin, frame->localOrigin, localOrigin );
	int partialFog = 0;
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		if ( localOrigin[0] - frame->radius >= fog->bounds[0][0] 
			&& localOrigin[0] + frame->radius <= fog->bounds[1][0] 
			&& localOrigin[1] - frame->radius >= fog->bounds[0][1]
			&& localOrigin[1] + frame->radius <= fog->bounds[1][1] 
			&& localOrigin[2] - frame->radius >= fog->bounds[0][2]
			&& localOrigin[2] + frame->radius <= fog->bounds[1][2] ) 
		{//totally inside it
			return i;
			break;
		}
		if ( ( localOrigin[0] - frame->radius >= fog->bounds[0][0] && localOrigin[1] - frame->radius >= fog->bounds[0][1] && localOrigin[2] - frame->radius >= fog->bounds[0][2] &&
			localOrigin[0] - frame->radius <= fog->bounds[1][0] && localOrigin[1] - frame->radius <= fog->bounds[1][1] && localOrigin[2] - frame->radius <= fog->bounds[1][2] ) || 
			( localOrigin[0] + frame->radius >= fog->bounds[0][0] && localOrigin[1] + frame->radius >= fog->bounds[0][1] && localOrigin[2] + frame->radius >= fog->bounds[0][2] &&
			localOrigin[0] + frame->radius <= fog->bounds[1][0] && localOrigin[1] + frame->radius <= fog->bounds[1][1] && localOrigin[2] + frame->radius <= fog->bounds[1][2] ) ) 
		{//partially inside it
			if ( tr.refdef.fogIndex == i || R_FogParmsMatch( tr.refdef.fogIndex, i ) )
			{//take new one only if it's the same one that the viewpoint is in
				return i;
				break;
			}
			else if ( !partialFog )
			{//first partialFog
				partialFog = i;
			}
		}
	}
	//if all else fails, return the first partialFog
	return partialFog;
}

/*
==============
R_AddAnimSurfaces
==============
*/
void R_AddAnimSurfaces( trRefEntity_t *ent ) {
	md4Header_t		*header;
	md4Surface_t	*surface;
	md4LOD_t		*lod;
	shader_t		*shader = 0;
	shader_t		*cust_shader = 0;
	int				fogNum = 0;
	qboolean		personalModel;
	int				cull;
	int				i, whichLod;

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	if ( ent->e.renderfx & RF_CAP_FRAMES) {
		if (ent->e.frame > tr.currentModel->md4->numFrames-1)
			ent->e.frame = tr.currentModel->md4->numFrames-1;
		if (ent->e.oldframe > tr.currentModel->md4->numFrames-1)
			ent->e.oldframe = tr.currentModel->md4->numFrames-1;
	}
	else if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->md4->numFrames;
		ent->e.oldframe %= tr.currentModel->md4->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( (ent->e.frame >= tr.currentModel->md4->numFrames) 
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= tr.currentModel->md4->numFrames)
		|| (ent->e.oldframe < 0) ) 
	{
#ifdef _DEBUG
			ri.Printf (PRINT_ALL, "R_AddAnimSurfaces: no such frame %d to %d for '%s'\n",
#else
			ri.Printf (PRINT_DEVELOPER, "R_AddAnimSurfaces: no such frame %d to %d for '%s'\n",				
#endif
			ent->e.oldframe, ent->e.frame,
			tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	header = tr.currentModel->md4;

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_ACullModel ( header, ent );
	if ( cull == CULL_OUT ) {
		return;
	}

	//
	// compute LOD
	//
	lod = (md4LOD_t *)( (byte *)header + header->ofsLODs );
	whichLod = R_ComputeLOD( ent );
	for ( i = 0; i < whichLod; i++)
	{
		lod = (md4LOD_t*)( (byte *)lod + lod->ofsEnd );
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// see if we are in a fog volume
	//
	fogNum = R_AComputeFogNum( header, ent );


	//
	// draw all surfaces
	//
	cust_shader = R_GetShaderByHandle( ent->e.customShader );


	surface = (md4Surface_t *)( (byte *)lod + lod->ofsSurfaces );
	for ( i = 0 ; i < lod->numSurfaces ; i++ ) {
		if ( ent->e.customShader ) {
			shader = cust_shader;
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int		j;
			
			skin = R_GetSkinByHandle( ent->e.customSkin );
			
			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
				// the names have both been lowercased
				if ( !strcmp( skin->surfaces[j]->name, surface->name ) ) {
					shader = skin->surfaces[j]->shader;
					break;
				}
			}
		} else {
			shader = R_GetShaderByHandle( surface->shaderIndex );
		}
		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
			&& r_shadows->integer == 2 
			&& fogNum == 0
			&& (ent->e.renderfx & RF_SHADOW_PLANE )
			&& !(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) ) 
			&& shader->sort == SS_OPAQUE ) {
#ifdef _NPATCH
			R_AddDrawSurf( (surfaceType_t *)surface, tr.shadowShader, 0, qfalse, 0 );
#else
			R_AddDrawSurf( (surfaceType_t *)surface, tr.shadowShader, 0, qfalse );
#endif // _NPATCH
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			&& fogNum == 0
			&& (ent->e.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE ) {
#ifdef _NPATCH
			R_AddDrawSurf( (surfaceType_t *)surface, tr.projectionShadowShader, 0, qfalse, 0 );
#else
			R_AddDrawSurf( (surfaceType_t *)surface, tr.projectionShadowShader, 0, qfalse );
#endif // _NPATCH
		}

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
#ifdef _NPATCH
			R_AddDrawSurf( (surfaceType_t *)surface, shader, fogNum, qfalse, 0 );
#else
			R_AddDrawSurf( (surfaceType_t *)surface, shader, fogNum, qfalse );
#endif // _NPATCH
		}

		surface = (md4Surface_t *)( (byte *)surface + surface->ofsEnd );
	}
}


/*
==============
RB_SurfaceAnim
==============
*/
void RB_SurfaceAnim( md4Surface_t *surface ) {
	int				i, j, k;
	float			frontlerp, backlerp;
	int				*triangles;
	int				indexes;
	int				baseIndex, baseVertex;
	int				numVerts;
	md4Vertex_t		*v;
	md4Bone_t		bones[MD4_MAX_BONES];
	md4Bone_t		tbone[2];
	md4Bone_t		*bonePtr, *bone;
	md4Header_t		*header;
	md4Frame_t		*frame=0;
	md4Frame_t		*oldFrame=0;
	md4CompFrame_t	*cframe=0;
	md4CompFrame_t	*coldFrame=0;
	int				frameSize;
	qboolean		compressed;


	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
		frontlerp = 1;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
		frontlerp = 1.0 - backlerp;
	}
	header = (md4Header_t *)((byte *)surface + surface->ofsHeader);

	if (header->ofsFrames<0) // Compressed
	{
		compressed = qtrue;
		frameSize = (int)( &((md4CompFrame_t *)0)->bones[ header->numBones ] );		
		cframe = (md4CompFrame_t *)((byte *)header - header->ofsFrames + backEnd.currentEntity->e.frame * frameSize );
		coldFrame = (md4CompFrame_t *)((byte *)header - header->ofsFrames + backEnd.currentEntity->e.oldframe * frameSize );
	}
	else
	{
		compressed = qfalse;
		frameSize = (int)( &((md4Frame_t *)0)->bones[ header->numBones ] );
		frame = (md4Frame_t *)((byte *)header + header->ofsFrames + 
			backEnd.currentEntity->e.frame * frameSize );
		oldFrame = (md4Frame_t *)((byte *)header + header->ofsFrames + 
			backEnd.currentEntity->e.oldframe * frameSize );
	}



	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	indexes = surface->numTriangles * 3;
	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !backlerp && !compressed)
		// no lerping needed
		bonePtr = frame->bones;
	else 
	{
		bonePtr = bones;
		if (compressed)
		{
			for ( i = 0 ; i < header->numBones ; i++ ) 
			{
				if ( !backlerp )
					MC_UnCompress(bonePtr[i].matrix,cframe->bones[i].Comp);
				else
				{
					MC_UnCompress(tbone[0].matrix,cframe->bones[i].Comp);
					MC_UnCompress(tbone[1].matrix,coldFrame->bones[i].Comp);
					for ( j = 0 ; j < 12 ; j++ ) 
						((float *)&bonePtr[i])[j] = frontlerp * ((float *)&tbone[0])[j]
							+ backlerp * ((float *)&tbone[1])[j];
				}
			}
		}
		else
		{
			for ( i = 0 ; i < header->numBones*12 ; i++ ) 
				((float *)bonePtr)[i] = frontlerp * ((float *)frame->bones)[i]
					+ backlerp * ((float *)oldFrame->bones)[i];
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts);
	for ( j = 0; j < numVerts; j++ ) {
		vec3_t	tempVert, tempNormal;
		md4Weight_t	*w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) {
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

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (md4Vertex_t *)&v->weights[v->numWeights];
	}

	tess.numVertexes += surface->numVerts;
}