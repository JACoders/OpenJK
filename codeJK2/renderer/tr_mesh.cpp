// tr_mesh.c: triangle model functions

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"




#include "tr_local.h"
#include "MatComp.h"

float ProjectRadius( float r, vec3_t location )
{
	float pr;
	float dist;
	float c;
	vec3_t	p;
	float	projected[4];

	c = DotProduct( tr.viewParms.or.axis[0], tr.viewParms.or.origin );
	dist = DotProduct( tr.viewParms.or.axis[0], location ) - c;

	if ( dist <= 0 )
		return 0;

	p[0] = 0;
	p[1] = fabs( r );
	p[2] = -dist;

	projected[0] = p[0] * tr.viewParms.projectionMatrix[0] + 
		           p[1] * tr.viewParms.projectionMatrix[4] +
				   p[2] * tr.viewParms.projectionMatrix[8] +
				   tr.viewParms.projectionMatrix[12];

	projected[1] = p[0] * tr.viewParms.projectionMatrix[1] + 
		           p[1] * tr.viewParms.projectionMatrix[5] +
				   p[2] * tr.viewParms.projectionMatrix[9] +
				   tr.viewParms.projectionMatrix[13];

	projected[2] = p[0] * tr.viewParms.projectionMatrix[2] + 
		           p[1] * tr.viewParms.projectionMatrix[6] +
				   p[2] * tr.viewParms.projectionMatrix[10] +
				   tr.viewParms.projectionMatrix[14];

	projected[3] = p[0] * tr.viewParms.projectionMatrix[3] + 
		           p[1] * tr.viewParms.projectionMatrix[7] +
				   p[2] * tr.viewParms.projectionMatrix[11] +
				   tr.viewParms.projectionMatrix[15];


	pr = projected[1] / projected[3];

	if ( pr > 1.0f )
		pr = 1.0f;

	return pr;
}

/*
=============
R_CullModel
=============
*/
static int R_CullModel( md3Header_t *header, trRefEntity_t *ent ) {
	vec3_t		bounds[2];
	md3Frame_t	*oldFrame, *newFrame;
	int			i;

	// compute frame pointers
	newFrame = ( md3Frame_t * ) ( ( byte * ) header + header->ofsFrames ) + ent->e.frame;
	oldFrame = ( md3Frame_t * ) ( ( byte * ) header + header->ofsFrames ) + ent->e.oldframe;

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
R_ComputeLOD

=================
*/
int R_ComputeLOD( trRefEntity_t *ent ) {
	float radius;
	float flod;
	float projectedRadius;
	int		lod;

	if ( tr.currentModel->numLods < 2 )
	{	// model has only 1 LOD level, skip computations and bias
		return(0);
	}

	// multiple LODs exist, so compute projected bounding sphere
	// and use that as a criteria for selecting LOD
	if ( tr.currentModel->md3[0] ) {	//normal md3
		md3Frame_t *frame;
		frame = ( md3Frame_t * ) ( ( ( unsigned char * ) tr.currentModel->md3[0] ) + tr.currentModel->md3[0]->ofsFrames );
		frame += ent->e.frame;
		radius = RadiusFromBounds( frame->bounds[0], frame->bounds[1] );
	}
	else {	//assume md4
		int frameSize;
		if (tr.currentModel->md4->ofsFrames<0) // Compressed
		{
			md4CompFrame_t	*frame;
			frameSize = (int)( &((md4CompFrame_t *)0)->bones[ tr.currentModel->md4->numBones ] );		
			frame = (md4CompFrame_t *)((byte *)tr.currentModel->md4 - tr.currentModel->md4->ofsFrames + ent->e.frame * frameSize );
			radius = RadiusFromBounds( frame->bounds[0], frame->bounds[1] );
		}
		else
		{
			md4Frame_t	*frame;
			frameSize = (int)( &((md4Frame_t *)0)->bones[ tr.currentModel->md4->numBones ] );		
			frame = (md4Frame_t *)((byte *)tr.currentModel->md4 + tr.currentModel->md4->ofsFrames + ent->e.frame * frameSize );
			radius = RadiusFromBounds( frame->bounds[0], frame->bounds[1] );
		}
	}

	if ( ( projectedRadius = ProjectRadius( radius, ent->e.origin ) ) != 0 )
	{
		flod = 1.0f - projectedRadius * r_lodscale->value;
		flod *= tr.currentModel->numLods;
	}
	else
	{	// object intersects near view plane, e.g. view weapon
		flod = 0;
	}

	lod = myftol( flod );

	if ( lod < 0 ) {
		lod = 0;
	} else if ( lod >= tr.currentModel->numLods ) {
		lod = tr.currentModel->numLods - 1;
	}
	
	lod += r_lodbias->integer;
	if ( lod >= tr.currentModel->numLods )
		lod = tr.currentModel->numLods - 1;
	if ( lod < 0 )
		lod = 0;

	return lod;
}

/*
=================
R_ComputeFogNum

=================
*/
static int R_ComputeFogNum( md3Header_t *header, trRefEntity_t *ent ) {
	int				i;
	fog_t			*fog;
	md3Frame_t		*md3Frame;
	vec3_t			localOrigin;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	if (tr.refdef.doLAGoggles)
	{
		return tr.world->numfogs;
	}


	// FIXME: non-normalized axis issues
	md3Frame = ( md3Frame_t * ) ( ( byte * ) header + header->ofsFrames ) + ent->e.frame;
	VectorAdd( ent->e.origin, md3Frame->localOrigin, localOrigin );

	int partialFog = 0;
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		if ( localOrigin[0] - md3Frame->radius >= fog->bounds[0][0] 
			&& localOrigin[0] + md3Frame->radius <= fog->bounds[1][0] 
			&& localOrigin[1] - md3Frame->radius >= fog->bounds[0][1]
			&& localOrigin[1] + md3Frame->radius <= fog->bounds[1][1] 
			&& localOrigin[2] - md3Frame->radius >= fog->bounds[0][2]
			&& localOrigin[2] + md3Frame->radius <= fog->bounds[1][2] ) 
		{//totally inside it
			return i;
			break;
		}
		if ( ( localOrigin[0] - md3Frame->radius >= fog->bounds[0][0] && localOrigin[1] - md3Frame->radius >= fog->bounds[0][1] && localOrigin[2] - md3Frame->radius >= fog->bounds[0][2] &&
				localOrigin[0] - md3Frame->radius <= fog->bounds[1][0] && localOrigin[1] - md3Frame->radius <= fog->bounds[1][1] && localOrigin[2] - md3Frame->radius <= fog->bounds[1][2]) ||
			( localOrigin[0] + md3Frame->radius >= fog->bounds[0][0] && localOrigin[1] + md3Frame->radius >= fog->bounds[0][1] && localOrigin[2] + md3Frame->radius >= fog->bounds[0][2] &&
				localOrigin[0] + md3Frame->radius <= fog->bounds[1][0] && localOrigin[1] + md3Frame->radius <= fog->bounds[1][1] && localOrigin[2] + md3Frame->radius <= fog->bounds[1][2] ) ) 
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
=================
R_AddMD3Surfaces

=================
*/
void R_AddMD3Surfaces( trRefEntity_t *ent ) {
	int				i;
	md3Header_t		*header = 0;
	md3Surface_t	*surface = 0;
	md3Shader_t		*md3Shader = 0;
	shader_t		*shader = 0;
	shader_t		*main_shader = 0;
	int				cull;
	int				lod;
	int				fogNum;
	qboolean		personalModel;

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	if ( ent->e.renderfx & RF_CAP_FRAMES) {
		if (ent->e.frame > tr.currentModel->md3[0]->numFrames-1)
			ent->e.frame = tr.currentModel->md3[0]->numFrames-1;
		if (ent->e.oldframe > tr.currentModel->md3[0]->numFrames-1)
			ent->e.oldframe = tr.currentModel->md3[0]->numFrames-1;
	}
	else if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->md3[0]->numFrames;
		ent->e.oldframe %= tr.currentModel->md3[0]->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( (ent->e.frame >= tr.currentModel->md3[0]->numFrames) 
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= tr.currentModel->md3[0]->numFrames)
		|| (ent->e.oldframe < 0) ) 
	{
			ri.Printf (PRINT_ALL, "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
				ent->e.oldframe, ent->e.frame,
				tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	//
	// compute LOD
	//
	lod = R_ComputeLOD( ent );

	header = tr.currentModel->md3[lod];

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel ( header, ent );
	if ( cull == CULL_OUT ) {
		return;
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
	fogNum = R_ComputeFogNum( header, ent );

	//
	// draw all surfaces
	//
	main_shader = R_GetShaderByHandle( ent->e.customShader );

	surface = (md3Surface_t *)( (byte *)header + header->ofsSurfaces );
	for ( i = 0 ; i < header->numSurfaces ; i++ ) {

		if ( ent->e.customShader ) {// a little more efficient
			shader = main_shader;
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
		} else if ( surface->numShaders <= 0 ) {
			shader = tr.defaultShader;
		} else {
			md3Shader = (md3Shader_t *) ( (byte *)surface + surface->ofsShaders );
			md3Shader += ent->e.skinNum % surface->numShaders;
			shader = tr.shaders[ md3Shader->shaderIndex ];
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

		surface = (md3Surface_t *)( (byte *)surface + surface->ofsEnd );
	}

}

