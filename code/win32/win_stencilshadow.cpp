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

#include <xgraphics.h>
#include <xgmath.h>

#include "shader_constants.h"



StencilShadow StencilShadower;


StencilShadow::StencilShadow()
{
	m_dwVertexShaderShadow = 0;

	for(int i = 0; i < SHADER_MAX_VERTEXES / 2; i++)
	{
		m_extrusionIndicators[i] = 1.0f;
	}
}


StencilShadow::~StencilShadow()
{
	if(m_dwVertexShaderShadow)
		glw_state->device->DeleteVertexShader(m_dwVertexShaderShadow);
}

extern const char *Sys_RemapPath( const char *filename );

bool StencilShadow::Initialize()
{
	// Create a vertex shader
    DWORD dwVertexDecl[] =
    {
        D3DVSD_STREAM( 0 ),
        D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),     // v0 = Position
		D3DVSD_REG( 1, D3DVSDT_FLOAT1 ),     // v1 = Extrusion determinant
        D3DVSD_END()
    };

    if(!( CreateVertexShader(Sys_RemapPath("base\\media\\shadow.xvu"), dwVertexDecl, &m_dwVertexShaderShadow)))
		return false;

	return true;
}


void StencilShadow::AddEdge( unsigned short i1, unsigned short i2, byte facing )
{
#ifndef DISABLE_STENCILSHADOW
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
#endif
}


void StencilShadow::BuildEdges()
{
#ifndef DISABLE_STENCILSHADOW
	int		i;
	int		c;
	int		j;
	unsigned short i2;
	int		numTris;
	unsigned short o1, o2, o3;

	int hit[2];
	int c2, k;

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	m_nIndexes = 0;
	m_nIndexesCap = 0;

	for ( i = 0 ; i < tess.numVertexes ; i++ ) 
	{
		c = m_numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) 
		{
			if ( !m_edgeDefs[ i ][ j ].facing ) 
			{
				continue;
			}

			/*i2 = m_edgeDefs[ i ][ j ].i2;
			
			m_shadowIndexes[m_nIndexes++] = i;
			m_shadowIndexes[m_nIndexes++] = i + tess.numVertexes;
			m_shadowIndexes[m_nIndexes++] = i2 + tess.numVertexes;
			m_shadowIndexes[m_nIndexes++] = i2;*/

			hit[0] = 0;
			hit[1] = 0;

			i2 = m_edgeDefs[ i ][ j ].i2;
			c2 = m_numEdgeDefs[ i2 ];
			for ( k = 0 ; k < c2 ; k++ ) {
				if ( m_edgeDefs[ i2 ][ k ].i2 == i ) {
					hit[ m_edgeDefs[ i2 ][ k ].facing ]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if ( hit[ 1 ] == 0 ) {
				m_shadowIndexes[m_nIndexes++] = i;
				m_shadowIndexes[m_nIndexes++] = i + tess.numVertexes;
				m_shadowIndexes[m_nIndexes++] = i2 + tess.numVertexes;
				m_shadowIndexes[m_nIndexes++] = i2;
			}
		}
	}

	if(!m_nIndexes)
		return;

#ifdef _STENCIL_REVERSE
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

		m_shadowIndexesCap[m_nIndexesCap++] = o1;
		m_shadowIndexesCap[m_nIndexesCap++] = o2;
		m_shadowIndexesCap[m_nIndexesCap++] = o3;
		m_shadowIndexesCap[m_nIndexesCap++] = o3 + tess.numVertexes;
		m_shadowIndexesCap[m_nIndexesCap++] = o2 + tess.numVertexes;
		m_shadowIndexesCap[m_nIndexesCap++] = o1 + tess.numVertexes;
	}
#endif // _STENCIL_REVERSE

#endif
}


bool StencilShadow::BuildFromLight()
{
#ifndef DISABLE_STENCILSHADOW
	int		i;
	int		numTris;
	vec3_t	lightDir, ground;
	float   d;

	// we can only do this if we have enough space in the vertex buffers
	if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
		return false;
	}

	//controlled method - try to keep shadows in range so they don't show through so much -rww
	//VectorCopy( backEnd.currentEntity->shadowDir, lightDir );
	//
	//ground[0] = backEnd.ori.axis[0][2];
	//ground[1] = backEnd.ori.axis[1][2];
	//ground[2] = backEnd.ori.axis[2][2];

	//d = DotProduct( lightDir, ground );
	//// don't let the shadows get too long or go negative
	//if ( d < 0.8 ) {
	//	VectorMA( lightDir, (0.8 - d), ground, lightDir );
	//	d = DotProduct( lightDir, ground );
	//}
	//d = 1.0 / d;

	//lightDir[0] = lightDir[0] * d;
	//lightDir[1] = lightDir[1] * d;
	//lightDir[2] = lightDir[2] * d;

	//VectorNormalize(lightDir);

	vec3_t entLight;
	VectorCopy( backEnd.currentEntity->lightDir, entLight );
	entLight[2] = 0.0f;
	VectorNormalize(entLight);

	//Oh well, just cast them straight down no matter what onto the ground plane.
	//This presets no chance of screwups and still looks better than a stupid
	//shader blob.
	VectorSet(lightDir, entLight[0]*0.3f, entLight[1]*0.3f, 1.0f);
	
	
	// Set the vertex shader constants
	D3DXVECTOR4 light = D3DXVECTOR4(lightDir[0], lightDir[1], lightDir[2], 1.0f);
	glw_state->device->SetVertexShaderConstant( CV_LIGHT_DIRECTION, light, 1 );

	glw_state->device->SetVertexShaderConstant( CV_SHADOW_FACTORS, D3DXVECTOR4(backEnd.ori.origin[0],
																			   backEnd.ori.origin[1],
																			   backEnd.ori.origin[2],
																			   1.0f), 1);
	glw_state->device->SetVertexShaderConstant( CV_SHADOW_PLANE, D3DXVECTOR4( backEnd.currentEntity->e.shadowPlane - 16.0f,
																			  backEnd.currentEntity->e.shadowPlane - 16.0f,
																			  backEnd.currentEntity->e.shadowPlane - 16.0f,
																			  1.0f), 1);
	
	// Create a second set of vertices to be projected
	memcpy(&tess.xyz[tess.numVertexes], &tess.xyz[0], sizeof(vec4_t) * tess.numVertexes);

	// decide which triangles face the light
	memset( m_numEdgeDefs, 0, sizeof(short) * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ ) 
	{
		short	i1, i2, i3;
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
#else
	return false;
#endif
}


void StencilShadow::RenderShadow()
{
#ifndef DISABLE_STENCILSHADOW
	DWORD lighting, fog, srcblend, destblend, alphablend, zwrite, zfunc, cullmode;

	GL_State(GLS_DEFAULT);

	glw_state->device->GetRenderState( D3DRS_LIGHTING, &lighting );
	glw_state->device->GetRenderState( D3DRS_FOGENABLE, &fog );
	glw_state->device->GetRenderState( D3DRS_SRCBLEND, &srcblend );
	glw_state->device->GetRenderState( D3DRS_DESTBLEND, &destblend );
	glw_state->device->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphablend );
	glw_state->device->GetRenderState( D3DRS_ZWRITEENABLE, &zwrite );
	glw_state->device->GetRenderState( D3DRS_ZFUNC, &zfunc );
	glw_state->device->GetRenderState( D3DRS_CULLMODE, &cullmode );

	pVerts = NULL;
	pExtrusions = NULL;

	GL_Bind( tr.whiteImage );

	glw_state->device->SetRenderState( D3DRS_LIGHTING, FALSE );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, FALSE );

	glw_state->device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );

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
#ifdef _STENCIL_REVERSE
	glw_state->device->SetRenderState( D3DRS_STENCILZFAIL,  D3DSTENCILOP_INCR );
    glw_state->device->SetRenderState( D3DRS_STENCILFAIL,   D3DSTENCILOP_KEEP );
	glw_state->device->SetRenderState( D3DRS_STENCILPASS,   D3DSTENCILOP_KEEP );
#else
	glw_state->device->SetRenderState( D3DRS_STENCILZFAIL,  D3DSTENCILOP_KEEP );
    glw_state->device->SetRenderState( D3DRS_STENCILFAIL,   D3DSTENCILOP_KEEP );
	glw_state->device->SetRenderState( D3DRS_STENCILPASS,   D3DSTENCILOP_INCR );	
#endif

    // If ztest passes, inc/decrement stencil buffer value
    glw_state->device->SetRenderState( D3DRS_STENCILREF,       0x1 );
    glw_state->device->SetRenderState( D3DRS_STENCILMASK,      0x7f ); //0xffffffff );
    glw_state->device->SetRenderState( D3DRS_STENCILWRITEMASK, 0x7f ); //0xffffffff );

    // Make sure that no pixels get drawn to the frame buffer
    glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    glw_state->device->SetRenderState( D3DRS_COLORWRITEENABLE, 0 );

	glw_state->device->SetTexture(0, NULL);
	glw_state->device->SetTexture(1, NULL);

	// Compute the matrix set
    XGMATRIX matComposite, matProjectionViewport, matWorld;
	glw_state->device->GetProjectionViewportMatrix( &matProjectionViewport );

	XGMatrixMultiply( &matComposite, (XGMATRIX*)glw_state->matrixStack[glwstate_t::MatrixMode_Model]->GetTop(), &matProjectionViewport );

	// Transpose and set the composite matrix.
	XGMatrixTranspose( &matComposite, &matComposite );
	glw_state->device->SetVertexShaderConstant( CV_WORLDVIEWPROJ_0, &matComposite, 4 );

	// Set viewport offsets.
	float fViewportOffsets[4] = { 0.53125f, 0.53125f, 0.0f, 0.0f };
	glw_state->device->SetVertexShaderConstant( CV_VIEWPORT_OFFSETS, &fViewportOffsets, 1 );

	glw_state->device->SetVertexShader(m_dwVertexShaderShadow);

#ifdef _STENCIL_REVERSE
	qglCullFace( GL_FRONT );
#else
	qglCullFace( GL_BACK );
#endif

	BuildEdges();

	// Draw front-side of shadow volume in stencil/z only
	if(m_nIndexes)
        renderObject_Shadow( D3DPT_QUADLIST, m_nIndexes, m_shadowIndexes );
#ifdef _STENCIL_REVERSE
	if(m_nIndexesCap)
        renderObject_Shadow( D3DPT_TRIANGLELIST, m_nIndexesCap, m_shadowIndexesCap );
#endif

	// Now reverse cull order so back sides of shadow volume are written.
#ifdef _STENCIL_REVERSE
	qglCullFace( GL_BACK );
#else
    qglCullFace( GL_FRONT );
#endif

    // Decrement stencil buffer value
#ifdef _STENCIL_REVERSE
	glw_state->device->SetRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR );
#else
	glw_state->device->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );
#endif

	// Draw back-side of shadow volume in stencil/z only
	if(m_nIndexes)
        renderObject_Shadow( D3DPT_QUADLIST, m_nIndexes, m_shadowIndexes );
#ifdef _STENCIL_REVERSE
	if(m_nIndexesCap)
        renderObject_Shadow( D3DPT_TRIANGLELIST, m_nIndexesCap, m_shadowIndexesCap );
#endif

	// Restore render states
	glw_state->device->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL );

    glw_state->device->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	glw_state->device->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
	glw_state->device->SetRenderState( D3DRS_LIGHTING, lighting );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, fog );
	glw_state->device->SetRenderState( D3DRS_SRCBLEND, srcblend );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, destblend );
	glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, alphablend );
	glw_state->device->SetRenderState( D3DRS_ZWRITEENABLE, zwrite );
	glw_state->device->SetRenderState( D3DRS_ZFUNC, zfunc );
	glw_state->device->SetRenderState( D3DRS_CULLMODE, cullmode );
#endif
}


void StencilShadow::FinishShadows()
{
#ifndef DISABLE_STENCILSHADOW

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
    glw_state->device->SetRenderState( D3DRS_STENCILMASK,      0x7f ); // New!
	glw_state->device->SetRenderState( D3DRS_STENCILWRITEMASK, 0x7f ); //255 );

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
    glw_state->device->SetRenderState( D3DRS_TEXTUREFACTOR, 0x50000000 );

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
#endif
}
