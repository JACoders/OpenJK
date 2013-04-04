//
//
// win_stencilshadow.cpp
//
// Stencil shadow computation/rendering
//
//

#include "../server/exe_headers.h"

#include "../renderer/tr_local.h"
#include "../renderer/tr_lightmanager.h"
#include "glw_win_dx8.h"
#include "win_local.h"

#include "win_stencilshadow.h"


StencilShadow StencilShadower;


StencilShadow::StencilShadow()
{
}


StencilShadow::~StencilShadow()
{
}


void StencilShadow::AddEdge( int i1, int i2, int facing )
{
    int		c;

	c = m_numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) 
	{
		Com_Printf("WARNING: MAX_EDGE_DEFS overflow!\n");
		return;		// overflow
	}
	m_edgeDefs[ i1 ][ c ].i2 = i2;
	m_edgeDefs[ i1 ][ c ].facing = facing;

	m_numEdgeDefs[ i1 ]++;
}


void StencilShadow::RenderEdges()
{
 //   int		i;
	//int		c, c2;
	//int		j, k;
	//int		i2;
	//int		c_edges, c_rejected;
	//int		hit[2];

	//// an edge is NOT a silhouette edge if its face doesn't face the light,
	//// or if it has a reverse paired edge that also faces the light.
	//// A well behaved polyhedron would have exactly two faces for each edge,
	//// but lots of models have dangling edges or overfanned edges
	//c_edges = 0;
	//c_rejected = 0;

	//for ( i = 0 ; i < tess.numVertexes ; i++ ) 
	//{
	//	c = m_numEdgeDefs[ i ];
	//	for ( j = 0 ; j < c ; j++ ) 
	//	{
	//		if ( !m_edgeDefs[ i ][ j ].facing ) 
	//		{
	//			continue;
	//		}

	//		hit[0] = 0;
	//		hit[1] = 0;

	//		i2 = m_edgeDefs[ i ][ j ].i2;
	//		c2 = m_numEdgeDefs[ i2 ];
	//		for ( k = 0 ; k < c2 ; k++ ) 
	//		{
	//			if ( m_edgeDefs[ i2 ][ k ].i2 == i ) 
	//			{
	//				hit[ m_edgeDefs[ i2 ][ k ].facing ]++;
	//			}
	//		}

	//		// if it doesn't share the edge with another front facing
	//		// triangle, it is a sil edge
	//		if ( hit[ 1 ] == 0 ) 
	//		{
	//			VectorCopy( tess.xyz[i],					 m_shadowVerts[0] );
	//			VectorCopy( tess.xyz[i + tess.numVertexes],  m_shadowVerts[1] );
	//			VectorCopy( tess.xyz[i2],					 m_shadowVerts[2] );
	//			VectorCopy( tess.xyz[i2 + tess.numVertexes], m_shadowVerts[3] );

	//			c_edges++;

	//			glw_state->device->SetVertexShader( D3DFVF_XYZ );
	//			glw_state->device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, m_shadowVerts, sizeof(vec3_t) );
	//		} 
	//		else 
	//		{
	//			c_rejected++;
	//		}
	//	}
	//}

	int		i;
	int		c;
	int		j;
	int		i2;
	int		c_edges, c_rejected;
	int		numTris;
	int		o1, o2, o3;

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	c_edges = 0;
	c_rejected = 0;

	int nVerts = 0, numPrims = 0;

	for ( i = 0 ; i < tess.numVertexes ; i++ ) 
	{
		c = m_numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) 
		{
			if ( !m_edgeDefs[ i ][ j ].facing ) 
			{
				continue;
			}

			//with this system we can still get edges shared by more than 2 tris which
			//produces artifacts including seeing the shadow through walls. So for now
			//we are going to render all edges even though it is a tiny bit slower. -rww
			i2 = m_edgeDefs[ i ][ j ].i2;
			VectorCopy( tess.xyz[i],					 m_shadowVerts[nVerts++] );
			VectorCopy( tess.xyz[i + tess.numVertexes],  m_shadowVerts[nVerts++] );
			VectorCopy( tess.xyz[i2 + tess.numVertexes], m_shadowVerts[nVerts++] );
			VectorCopy( tess.xyz[i2],					 m_shadowVerts[nVerts++] );
			numPrims++;
		}
	}

	if(!numPrims || !nVerts)
		return;

	glw_state->device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_DISABLE );

	glw_state->device->SetVertexShader( D3DFVF_XYZ );
	glw_state->device->DrawPrimitiveUP( D3DPT_QUADLIST, numPrims, m_shadowVerts, sizeof(vec3_t) );

	nVerts = 0;
	numPrims = 0;

	//Carmack Reverse<tm> method requires that volumes
	//be capped properly -rww
	numTris = tess.numIndexes / 3;

	for ( i = 0 ; i < numTris ; i++ )
	{
		if ( !m_facing[i] )
		{
			continue;
		}

		o1 = tess.indexes[ i*3 + 0 ];
		o2 = tess.indexes[ i*3 + 1 ];
		o3 = tess.indexes[ i*3 + 2 ];

		VectorCopy( tess.xyz[o1],  m_shadowVerts[nVerts++] );
		VectorCopy( tess.xyz[o2],  m_shadowVerts[nVerts++] );
		VectorCopy( tess.xyz[o3],  m_shadowVerts[nVerts++] );
		VectorCopy( tess.xyz[o3 + tess.numVertexes],  m_shadowVerts[nVerts++] );
		VectorCopy( tess.xyz[o2 + tess.numVertexes],  m_shadowVerts[nVerts++] );
		VectorCopy( tess.xyz[o1 + tess.numVertexes],  m_shadowVerts[nVerts++] );
		numPrims += 2;
	}

	glw_state->device->SetVertexShader( D3DFVF_XYZ );
	glw_state->device->DrawPrimitiveUP( D3DPT_TRIANGLELIST, numPrims, m_shadowVerts, sizeof(vec3_t) );
}


bool StencilShadow::BuildFromLight( VVdlight_t *dl )
{
 //   int		i;
	//int		numTris;
	//vec3_t	lightDir;
	//D3DXMATRIX matWorldInv;
	//D3DXVECTOR4 viewLightPos;

	//// we can only do this if we have enough space in the vertex buffers
	//if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
	//	return false;
	//}

	//// project vertexes away from light direction
	//for ( i = 0 ; i < tess.numVertexes ; i++ ) 
	//{
	//	// Get the light direction to the vertex
	//	VectorCopy( backEnd.currentEntity->lightDir, lightDir );

	//	VectorMA( tess.xyz[i], -512, lightDir, tess.xyz[i+tess.numVertexes] );
	//}

	int		i;
	int		numTris;
	vec3_t	lightDir, ground;
	float   d;

	// we can only do this if we have enough space in the vertex buffers
	if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
		return false;
	}

	//controlled method - try to keep shadows in range so they don't show through so much -rww
	vec3_t	worldxyz, ld;
	float	groundDist, extlength;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	
	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	lightDir[0] = lightDir[0] * d;
	lightDir[1] = lightDir[1] * d;
	lightDir[2] = lightDir[2] * d;

	VectorNormalize(lightDir);
	
	//Oh well, just cast them straight down no matter what onto the ground plane.
	//This presents no chance of screwups and still looks better than a stupid
	//shader blob.
	//VectorSet(lightDir, 0.0f, 0.0f, 1.0f);

	// project vertexes away from light direction
	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		//add or.origin to vert xyz to end up with world oriented coord, then figure
		//out the ground pos for the vert to project the shadow volume to
		//VectorAdd(tess.xyz[i], backEnd.ori.origin, worldxyz);
		//groundDist = worldxyz[2] - backEnd.currentEntity->e.shadowPlane;
		//groundDist += 2.0f; //fudge factor
		//VectorMA( tess.xyz[i], -groundDist, lightDir, tess.xyz[i+tess.numVertexes] );
		VectorMA( tess.xyz[i], -200.0f, lightDir, tess.xyz[i+tess.numVertexes] );
	}


	// decide which triangles face the light
	memset( m_numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ ) 
	{
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		d = DotProduct( normal, lightDir );
		if ( d > 0 ) {
			m_facing[ i ] = 1;
		} else {
			m_facing[ i ] = 0;
		}

		// create the edges
		AddEdge( i1, i2, m_facing[ i ] );
		AddEdge( i2, i3, m_facing[ i ] );
		AddEdge( i3, i1, m_facing[ i ] );
	}

	return true;
}


void StencilShadow::RenderShadow()
{
	DWORD lighting, fog, srcblend, destblend, alphablend, zwrite, zfunc;

	glw_state->device->GetRenderState( D3DRS_LIGHTING, &lighting );
	glw_state->device->GetRenderState( D3DRS_FOGENABLE, &fog );
	glw_state->device->GetRenderState( D3DRS_SRCBLEND, &srcblend );
	glw_state->device->GetRenderState( D3DRS_DESTBLEND, &destblend );
	glw_state->device->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphablend );
	glw_state->device->GetRenderState( D3DRS_ZWRITEENABLE, &zwrite );
	glw_state->device->GetRenderState( D3DRS_ZFUNC, &zfunc );

	GL_Bind( tr.whiteImage );

	glw_state->device->SetRenderState( D3DRS_LIGHTING, FALSE );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, FALSE );

    // Disable z-buffer writes (note: z-testing still occurs), and enable the
    // stencil-buffer
    glw_state->device->SetRenderState( D3DRS_ZWRITEENABLE,  FALSE );
    glw_state->device->SetRenderState( D3DRS_STENCILENABLE, TRUE );

    // Don't bother with interpolating color
    glw_state->device->SetRenderState( D3DRS_SHADEMODE,     D3DSHADE_FLAT );

	glw_state->device->SetRenderState( D3DRS_ZFUNC,			D3DCMP_LESS );

    // Set up stencil compare function, reference value, and masks.
    // Stencil test passes if ((ref & mask) cmpfn (stencil & mask)) is true.
    // Note: since we set up the stencil-test to always pass, the STENCILFAIL
    // renderstate is really not needed.
    glw_state->device->SetRenderState( D3DRS_STENCILFUNC,   D3DCMP_ALWAYS );
    glw_state->device->SetRenderState( D3DRS_STENCILZFAIL,  D3DSTENCILOP_INCR );
    glw_state->device->SetRenderState( D3DRS_STENCILFAIL,   D3DSTENCILOP_KEEP );

    // If ztest passes, inc/decrement stencil buffer value
    glw_state->device->SetRenderState( D3DRS_STENCILREF,       0x1 );
    glw_state->device->SetRenderState( D3DRS_STENCILMASK,      0xffffffff );
    glw_state->device->SetRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );
    glw_state->device->SetRenderState( D3DRS_STENCILPASS,      D3DSTENCILOP_KEEP );

    // Make sure that no pixels get drawn to the frame buffer
    glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    glw_state->device->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ZERO );
    glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	glw_state->device->SetTransform(D3DTS_VIEW, 
			glw_state->matrixStack[glwstate_t::MatrixMode_Model]->GetTop());

	glw_state->device->SetTexture(0, NULL);
	glw_state->device->SetTexture(1, NULL);

	qglCullFace( GL_FRONT );

    // Draw front-side of shadow volume in stencil/z only
    RenderEdges();

    // Now reverse cull order so back sides of shadow volume are written.
    qglCullFace( GL_BACK );

    // Decrement stencil buffer value
    glw_state->device->SetRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR );

    // Draw back-side of shadow volume in stencil/z only
    RenderEdges();

    // Restore render states
    glw_state->device->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	glw_state->device->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
	glw_state->device->SetRenderState( D3DRS_LIGHTING, lighting );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, fog );
	glw_state->device->SetRenderState( D3DRS_SRCBLEND, srcblend );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, destblend );
	glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, alphablend );
	glw_state->device->SetRenderState( D3DRS_ZWRITEENABLE, zwrite );
	glw_state->device->SetRenderState( D3DRS_ZFUNC, zfunc );
	glw_state->device->SetRenderState( D3DRS_CULLMODE,  D3DCULL_CCW );
}


void StencilShadow::FinishShadows()
{
	DWORD lighting, fog, srcblend, destblend, alphablend;

	glw_state->device->GetRenderState( D3DRS_LIGHTING, &lighting );
	glw_state->device->GetRenderState( D3DRS_FOGENABLE, &fog );
	glw_state->device->GetRenderState( D3DRS_SRCBLEND, &srcblend );
	glw_state->device->GetRenderState( D3DRS_DESTBLEND, &destblend );
	glw_state->device->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphablend );

    // The stencilbuffer values indicates # of shadows that overlap each pixel.
    // We only want to draw pixels that are in shadow, which was set up in
    // RenderShadow() such that StencilBufferValue >= 1. In the Direct3D API, 
    // the stencil test is pseudo coded as:
    //    StencilRef CompFunc StencilBufferValue
    // so we set our renderstates with StencilRef = 1 and CompFunc = LESSEQUAL.
    glw_state->device->SetRenderState( D3DRS_STENCILENABLE, TRUE );
    glw_state->device->SetRenderState( D3DRS_STENCILREF,    0);//0x1 );
    glw_state->device->SetRenderState( D3DRS_STENCILFUNC,   D3DCMP_NOTEQUAL);//D3DCMP_LESSEQUAL );
	glw_state->device->SetRenderState( D3DRS_STENCILWRITEMASK, 255 );

    // Set renderstates (disable z-buffering and turn on alphablending)
    glw_state->device->SetRenderState( D3DRS_ZENABLE,          FALSE );
    glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    glw_state->device->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    // Set the hardware to draw black, alpha-blending pixels
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
    glw_state->device->SetRenderState( D3DRS_TEXTUREFACTOR, 0x7f000000 );

	glw_state->device->SetRenderState( D3DRS_FOGENABLE, FALSE );

    // Draw the big, darkening square
    static FLOAT v[4][4] = 
    {
        {   0 - 0.5f,   0 - 0.5f, 0.0f, 1.0f },
        { 640 - 0.5f,   0 - 0.5f, 0.0f, 1.0f }, 
        { 640 - 0.5f, 480 - 0.5f, 0.0f, 1.0f },
        {   0 - 0.5f, 480 - 0.5f, 0.0f, 1.0f },
    };

    glw_state->device->SetVertexShader( D3DFVF_XYZRHW );
    glw_state->device->DrawPrimitiveUP( D3DPT_QUADLIST, 1, v, sizeof(v[0]) );

    // Restore render states
    glw_state->device->SetRenderState( D3DRS_ZENABLE,          TRUE );
    glw_state->device->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
    glw_state->device->SetRenderState( D3DRS_LIGHTING, lighting );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, fog );
	glw_state->device->SetRenderState( D3DRS_SRCBLEND, srcblend );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, destblend );
	glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, alphablend );
}
