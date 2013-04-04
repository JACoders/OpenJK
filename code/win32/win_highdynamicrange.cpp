//
//
// Win_HighDynamicRange.cpp
//
// High dynamic range effect
//
//

#include "../server/exe_headers.h"

#include "../renderer/tr_local.h"
#include "glw_win_dx8.h"
#include "win_local.h"
#include "win_highdynamicrange.h"
#include "shader_constants.h"

#include <xgmath.h>
#include <xgraphics.h>

extern const char *Sys_RemapPath( const char *filename );

VVHighDynamicRange HDREffect;

VVHighDynamicRange::VVHighDynamicRange()
{
	m_bInitialized = false;

	m_dwHotBlurPixelShader = 0;
    m_dwExtractHotPixelShader = 0;
}


void VVHighDynamicRange::Initialize()
{
    // Create pixel shader
	if(!(CreatePixelShader(Sys_RemapPath("base\\media\\hotblur.xpu"), &m_dwHotBlurPixelShader)))
		return;

	if(!(CreatePixelShader(Sys_RemapPath("base\\media\\extracthot.xpu"), &m_dwExtractHotPixelShader)))
		return;

    // Get size of render target
    LPDIRECT3DSURFACE8 pRenderTarget;
    glw_state->device->GetRenderTarget( &pRenderTarget );
    D3DSURFACE_DESC descRenderTarget;
    pRenderTarget->GetDesc( &descRenderTarget );
    UINT Width = descRenderTarget.Width;
    UINT Height = descRenderTarget.Height;
    D3DFORMAT Format = descRenderTarget.Format;
    pRenderTarget->Release();

    // Create extract hot text
    glw_state->device->CreateTexture( Width >> 1, Height >> 1, 1,
                             D3DUSAGE_RENDERTARGET, Format,
                             0, &m_rpHotImage);

    // Make the size a factor of 2 smaller on each axis 
    glw_state->device->CreateTexture( Width >> 1, Height >> 2, 1,
                             D3DUSAGE_RENDERTARGET, Format,
                             0, &m_rpBlur[0]);

    glw_state->device->CreateTexture( Width >> 2, Height >> 2, 1,
                             D3DUSAGE_RENDERTARGET, Format,
                             0, &m_rpBlur[1]);

    // Set bloom scale
    m_fBloomScale = 1.00f;

	m_bInitialized = true;
}


void VVHighDynamicRange::Render()
{
	if(!m_bInitialized)
		return;

	DWORD lighting, fog, srcblend, destblend, alphablend, zwrite, zenable;

	glw_state->device->GetRenderState( D3DRS_LIGHTING, &lighting );
	glw_state->device->GetRenderState( D3DRS_FOGENABLE, &fog );
	glw_state->device->GetRenderState( D3DRS_SRCBLEND, &srcblend );
	glw_state->device->GetRenderState( D3DRS_DESTBLEND, &destblend );
	glw_state->device->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphablend );
	glw_state->device->GetRenderState( D3DRS_ZWRITEENABLE, &zwrite );
	glw_state->device->GetRenderState( D3DRS_ZENABLE, &zenable );

    HotBlur();                  // Blur the hot values in the backbuffer
    DrawHotBlur();        // Draw blurred hot values, add to scene

	glw_state->device->SetRenderState( D3DRS_LIGHTING, lighting );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, fog );
	glw_state->device->SetRenderState( D3DRS_SRCBLEND, srcblend );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, destblend );
	glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, alphablend );
	glw_state->device->SetRenderState( D3DRS_ZWRITEENABLE, zwrite );
	glw_state->device->SetRenderState( D3DRS_ZENABLE, zenable );
}


void VVHighDynamicRange::DrawHotBlur()
{
    if( !m_pBlur )
        return;

    LPDIRECT3DTEXTURE8 pTexture = m_pBlur;

    // Get size of backbuffer
    LPDIRECT3DSURFACE8 pRenderTarget;
    glw_state->device->GetRenderTarget( &pRenderTarget );
    D3DSURFACE_DESC descRenderTarget;
    pRenderTarget->GetDesc( &descRenderTarget );
    UINT Width = descRenderTarget.Width;
    UINT Height = descRenderTarget.Height;
    pRenderTarget->Release();

    // Texture coordinates in linear format textures go from 0 to n-1 rather
    // than the 0 to 1 that is used for swizzled textures.
    D3DSURFACE_DESC desc;
    pTexture->GetLevelDesc( 0, &desc );
    struct BACKGROUNDVERTEX { D3DXVECTOR4 p; FLOAT tu, tv; } v[4];
    v[0].p = D3DXVECTOR4( -0.5f,        -0.5f,         1.0f, 1.0f );
    v[0].tu = 0.0f;              v[0].tv = 0.0f;
    v[1].p = D3DXVECTOR4( Width - 0.5f, -0.5f,         1.0f, 1.0f );
    v[1].tu = (float)desc.Width; v[1].tv = 0.0f;
    v[2].p = D3DXVECTOR4( -0.5f,        Height - 0.5f, 1.0f, 1.0f );
    v[2].tu = 0.0f;              v[2].tv = (float)desc.Height;
    v[3].p = D3DXVECTOR4( Width - 0.5f, Height - 0.5f, 1.0f, 1.0f );
    v[3].tu = (float)desc.Width; v[3].tv = (float)desc.Height;

    // Set states
    glw_state->device->SetPixelShader( 0 );
    glw_state->device->SetTexture( 0, pTexture );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE2X );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MAXMIPLEVEL, 0 );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_NONE );
    glw_state->device->SetRenderState( D3DRS_ZENABLE, FALSE ); 
    glw_state->device->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
    
    XGCOLOR Blend(1.0f, 1.0f, 1.0f, 1.0f);
    Blend*= r_hdrbloom->value;//m_fBloomScale; // adjust blend amount
    glw_state->device->SetRenderState( D3DRS_TEXTUREFACTOR, Blend );

    // add if requested
    glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
    glw_state->device->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );    
    glw_state->device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
    glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
    
    // Render the screen-aligned quadrilateral
    glw_state->device->SetVertexShader( D3DFVF_XYZRHW|D3DFVF_TEX1 );
    glw_state->device->DrawVerticesUP( D3DPT_QUADSTRIP, 4, v, sizeof(BACKGROUNDVERTEX) );

	glw_state->device->SetRenderState( D3DRS_ZENABLE, TRUE );
}


void VVHighDynamicRange::FilterCopy( LPDIRECT3DTEXTURE8 pTextureDst,
                                 LPDIRECT3DTEXTURE8 pTextureSrc,
                                 UINT nSample, FilterSample rSample[],
                                 UINT nSuperSampleX, UINT nSuperSampleY,
                                 bool bCrap,
                                 RECT* pRectDst, RECT* pRectSrc
                                 )
{
    // Texture space pixel center == screen space pixel center
    glw_state->device->SetScreenSpaceOffset( -0.5f, -0.5f );

    // Save current render target and depth buffer
    LPDIRECT3DSURFACE8 pRenderTarget, pZBuffer;
    glw_state->device->GetRenderTarget( &pRenderTarget );
    glw_state->device->GetDepthStencilSurface( &pZBuffer );

    // Set destination as render target
    LPDIRECT3DSURFACE8 pSurface = NULL;
    pTextureDst->GetSurfaceLevel( 0, &pSurface );
    glw_state->device->SetRenderTarget( pSurface, NULL );  // no depth-buffering
    pSurface->Release();

    // Get descriptions of source and destination
    D3DSURFACE_DESC descSrc;
    pTextureSrc->GetLevelDesc( 0, &descSrc );
    D3DSURFACE_DESC descDst;
    pTextureDst->GetLevelDesc( 0, &descDst );

    // Setup rectangles if not specified on input
    RECT rectSrc = { 0, 0, descSrc.Width, descSrc.Height };
    if( pRectSrc == NULL ) pRectSrc = &rectSrc;
    RECT rectDst = { 0, 0, descDst.Width, descDst.Height };
    if( pRectDst == NULL )
    {
        // If the destination rectangle is not specified,
        // we change it to match the source rectangle
        rectDst.right = (pRectSrc->right - pRectSrc->left) / nSuperSampleX;
        rectDst.bottom = (pRectSrc->bottom - pRectSrc->top) / nSuperSampleY;
        pRectDst = &rectDst;
    }
    assert( (pRectDst->right - pRectDst->left) ==
            (pRectSrc->right - pRectDst->left) / (INT)nSuperSampleX );
    assert( (pRectDst->bottom - pRectDst->top) ==
            (pRectSrc->bottom - pRectDst->top) / (INT)nSuperSampleY );
    
    //Set render state for filtering
    glw_state->device->SetRenderState( D3DRS_LIGHTING, FALSE );
    glw_state->device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    glw_state->device->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
    glw_state->device->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
    glw_state->device->SetRenderState( D3DRS_STENCILENABLE, FALSE );
    glw_state->device->SetRenderState( D3DRS_FOGENABLE, FALSE );
    // On first rendering, copy new value over current render target contents
    glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    // Setup subsequent renderings to add to previous value
    glw_state->device->SetRenderState( D3DRS_BLENDOP,   D3DBLENDOP_ADD );    
    glw_state->device->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
    glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

    // Set texture state
    for( UINT xx = 0; xx < 4; xx++)
    {
        // Use our source texture for all four stages
        glw_state->device->SetTexture( xx, pTextureSrc);  
        glw_state->device->SetTextureStageState( xx, D3DTSS_COLOROP, D3DTOP_DISABLE );
        glw_state->device->SetTextureStageState( xx, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
        
        // Pass texture coords without transformation
        glw_state->device->SetTextureStageState( xx, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );  

        // Each texture has different tex coords
        glw_state->device->SetTextureStageState( xx, D3DTSS_TEXCOORDINDEX, xx ); 
        glw_state->device->SetTextureStageState( xx, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
        glw_state->device->SetTextureStageState( xx, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
        glw_state->device->SetTextureStageState( xx, D3DTSS_MAXMIPLEVEL, 0 );
        glw_state->device->SetTextureStageState( xx, D3DTSS_MIPFILTER, D3DTEXF_NONE );
        glw_state->device->SetTextureStageState( xx, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
        glw_state->device->SetTextureStageState( xx, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
        glw_state->device->SetTextureStageState( xx, D3DTSS_COLORKEYOP, D3DTCOLORKEYOP_DISABLE );
        glw_state->device->SetTextureStageState( xx, D3DTSS_COLORSIGN, 0 );
        glw_state->device->SetTextureStageState( xx, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE );
    }
    
    // Use hot blur pixel shader
    if(bCrap)
        glw_state->device->SetPixelShader( m_dwExtractHotPixelShader );      
    else
        glw_state->device->SetPixelShader( m_dwHotBlurPixelShader );       

    // For screen-space texture-mapped quadrilateral
    glw_state->device->SetVertexShader( D3DFVF_XYZRHW|D3DFVF_TEX4 );   

    // Prepare quadrilateral vertices
    float x0 = (float)pRectDst->left;
    float y0 = (float)pRectDst->top;
    float x1 = (float)pRectDst->right;
    float y1 = (float)pRectDst->bottom;
    struct Quad
    {
        float x, y, z, w1;
        struct uv
        {
            float u, v;
        }
        tex[4];   // each texture has different offset
    } 
    aQuad[4] =
    { //  X   Y     Z   1/W     u0  v0      u1  v1      u2  v2      u3  v3
        {x0, y0, 1.0f, 1.0f, }, // texture coords are set below
        {x1, y0, 1.0f, 1.0f, },
        {x0, y1, 1.0f, 1.0f, },
        {x1, y1, 1.0f, 1.0f, }
	};

    // Set rendering to just the destination rect
    glw_state->device->SetScissors( 1, FALSE, (D3DRECT *)pRectDst );

    // Draw a quad for each block of 4 filter coefficients
    float fOffsetScaleU = (float)nSuperSampleX; // offset for supersample 
    float fOffsetScaleV = (float)nSuperSampleY;
    float u0 = (float)pRectSrc->left;
    float v0 = (float)pRectSrc->top;
    float u1 = (float)pRectSrc->right;
    float v1 = (float)pRectSrc->bottom;


    if( XGIsSwizzledFormat( descSrc.Format ) )
    {
        float fWidthScale = 1.f / (float)descSrc.Width;
        float fHeightScale = 1.f / (float)descSrc.Height;
        fOffsetScaleU *= fWidthScale;
        fOffsetScaleV *= fHeightScale;
        u0 *= fWidthScale;
        v0 *= fHeightScale;
        u1 *= fWidthScale;
        v1 *= fHeightScale;
	}
    
    xx = 0; // current texture stage
    D3DCOLOR rColor[4];
    DWORD rPSInput[4];
    for( UINT iSample = 0; iSample < nSample; iSample++ )
    {
        // Set filter coefficients
        float fValue = rSample[iSample].fValue;
        if( fValue < 0.f )
        {
            rColor[xx] = D3DXCOLOR( -fValue, -fValue, -fValue, -fValue );
            rPSInput[xx] = PS_INPUTMAPPING_SIGNED_NEGATE |
                           ((xx % 2) ? PS_REGISTER_C1 : PS_REGISTER_C0);
		}
        else
        {
            rColor[xx] = D3DXCOLOR( fValue, fValue, fValue, fValue );
            rPSInput[xx] = PS_INPUTMAPPING_SIGNED_IDENTITY |
                           ((xx % 2) ? PS_REGISTER_C1 : PS_REGISTER_C0);
		}

        // Align supersamples with center of destination pixels
        float fOffsetX = rSample[iSample].fOffsetX;// * fOffsetScaleU;
        float fOffsetY = rSample[iSample].fOffsetY;// * fOffsetScaleV;
        aQuad[0].tex[xx].u = u0 + fOffsetX;
        aQuad[0].tex[xx].v = v0 + fOffsetY;
        aQuad[1].tex[xx].u = u1 + fOffsetX;
        aQuad[1].tex[xx].v = v0 + fOffsetY;
        aQuad[2].tex[xx].u = u0 + fOffsetX;
        aQuad[2].tex[xx].v = v1 + fOffsetY;
        aQuad[3].tex[xx].u = u1 + fOffsetX;
        aQuad[3].tex[xx].v = v1 + fOffsetY;
        
        xx++; // Go to next stage
        if( xx == 4 || iSample == nSample - 1 ) // max texture stages or last sample
        {
            // Zero out unused texture stage coefficients 
            // (Only for last filter sample, when number of samples is not divisible by 4)
            for( ; xx < 4; xx++)
            {
                glw_state->device->SetTexture( xx, NULL );
                rColor[xx] = 0;
                rPSInput[xx] = PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_REGISTER_ZERO;
            }
        
            // Set coefficients
            glw_state->device->SetRenderState( D3DRS_PSCONSTANT0_0, rColor[0] );
            glw_state->device->SetRenderState( D3DRS_PSCONSTANT1_0, rColor[1] );
            glw_state->device->SetRenderState( D3DRS_PSCONSTANT0_1, rColor[2] );
            glw_state->device->SetRenderState( D3DRS_PSCONSTANT1_1, rColor[3] );

			if(bCrap)
            {
            }
            else
            {

            // Remap coefficients to proper sign
            glw_state->device->SetRenderState(
                D3DRS_PSRGBINPUTS0,
                PS_COMBINERINPUTS( rPSInput[0] | PS_CHANNEL_RGB,
                                   PS_REGISTER_T0 | PS_CHANNEL_RGB | 
                                      PS_INPUTMAPPING_SIGNED_IDENTITY,
                                   rPSInput[1] | PS_CHANNEL_RGB,
                                   PS_REGISTER_T1 | PS_CHANNEL_RGB |
                                       PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            glw_state->device->SetRenderState(
                D3DRS_PSALPHAINPUTS0,
                PS_COMBINERINPUTS( rPSInput[0] | PS_CHANNEL_ALPHA,
                                   PS_REGISTER_T0 | PS_CHANNEL_ALPHA |
                                      PS_INPUTMAPPING_SIGNED_IDENTITY,
                                   rPSInput[1] | PS_CHANNEL_ALPHA,
                                   PS_REGISTER_T1 | PS_CHANNEL_ALPHA |
                                      PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            glw_state->device->SetRenderState(
                D3DRS_PSRGBINPUTS1,
                PS_COMBINERINPUTS( rPSInput[2] | PS_CHANNEL_RGB,
                                   PS_REGISTER_T2 | PS_CHANNEL_RGB |
                                       PS_INPUTMAPPING_SIGNED_IDENTITY,
                                   rPSInput[3] | PS_CHANNEL_RGB,
                                   PS_REGISTER_T3 | PS_CHANNEL_RGB |
                                       PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            glw_state->device->SetRenderState(
                D3DRS_PSALPHAINPUTS1,
                PS_COMBINERINPUTS( rPSInput[2] | PS_CHANNEL_ALPHA,
                                   PS_REGISTER_T2 | PS_CHANNEL_ALPHA |
                                       PS_INPUTMAPPING_SIGNED_IDENTITY,
                                   rPSInput[3] | PS_CHANNEL_ALPHA,
                                   PS_REGISTER_T3 | PS_CHANNEL_ALPHA |
                                       PS_INPUTMAPPING_SIGNED_IDENTITY ) );
			}
            
            // Draw the quad to filter the coefficients so far
            // One quad blends 4 textures
            glw_state->device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, aQuad, sizeof(Quad) ); 

             // On subsequent renderings, add to what's in the render target 
            glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
            xx = 0;
        }
	}

    // Clear texture stages
    for( xx=0; xx<4; xx++ )
    {
        glw_state->device->SetTexture( xx, NULL );
        glw_state->device->SetTextureStageState( xx, D3DTSS_COLOROP, D3DTOP_DISABLE );
        glw_state->device->SetTextureStageState( xx, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
        glw_state->device->SetTextureStageState( xx, D3DTSS_MIPMAPLODBIAS, 0 );
    }

    // Restore render target and zbuffer
    glw_state->device->SetRenderTarget( pRenderTarget, pZBuffer );

    if( pRenderTarget ) 
		pRenderTarget->Release();

    if( pZBuffer )      
		pZBuffer->Release();

    glw_state->device->SetScreenSpaceOffset( 0.0f, 0.0f );
}


void VVHighDynamicRange::ExtractHot( LPDIRECT3DTEXTURE8 pTextureDst,
                                 LPDIRECT3DTEXTURE8 pTextureSrc,
                                 UINT nSuperSampleX, UINT nSuperSampleY,
                                 RECT* pRectDst, RECT* pRectSrc )
{
	// Texture space pixel center == screen space pixel center
    glw_state->device->SetScreenSpaceOffset( -0.5f, -0.5f );

    // Save current render target and depth buffer
    LPDIRECT3DSURFACE8 pRenderTarget, pZBuffer;
    glw_state->device->GetRenderTarget( &pRenderTarget );
    glw_state->device->GetDepthStencilSurface( &pZBuffer );

	// Surface that has the depth-buffer as data, used as an RGBA texture:
	D3DSurface zBufferSurface;
	D3DSURFACE_DESC descZ;
	pZBuffer->GetDesc( &descZ );
	XGSetSurfaceHeader( descZ.Width, descZ.Height, D3DFMT_LIN_A8R8G8B8, &zBufferSurface, pZBuffer->Data, descZ.Width * 4 );

    // Set destination as render target
    LPDIRECT3DSURFACE8 pSurface = NULL;
    pTextureDst->GetSurfaceLevel( 0, &pSurface );
    glw_state->device->SetRenderTarget( pSurface, NULL );  // no depth-buffering
    pSurface->Release();

    // Get descriptions of source and destination
    D3DSURFACE_DESC descSrc;
    pTextureSrc->GetLevelDesc( 0, &descSrc );
    D3DSURFACE_DESC descDst;
    pTextureDst->GetLevelDesc( 0, &descDst );

    // Setup rectangles if not specified on input
    RECT rectSrc = { 0, 0, descSrc.Width, descSrc.Height };
    if( pRectSrc == NULL ) pRectSrc = &rectSrc;
    RECT rectDst = { 0, 0, descDst.Width, descDst.Height };
    if( pRectDst == NULL )
    {
        // If the destination rectangle is not specified,
        // we change it to match the source rectangle
        rectDst.right = (pRectSrc->right - pRectSrc->left) / nSuperSampleX;
        rectDst.bottom = (pRectSrc->bottom - pRectSrc->top) / nSuperSampleY;
        pRectDst = &rectDst;
    }
    assert( (pRectDst->right - pRectDst->left) ==
            (pRectSrc->right - pRectDst->left) / (INT)nSuperSampleX );
    assert( (pRectDst->bottom - pRectDst->top) ==
            (pRectSrc->bottom - pRectDst->top) / (INT)nSuperSampleY );
    
    //Set render state for filtering
    glw_state->device->SetRenderState( D3DRS_LIGHTING, FALSE );
    glw_state->device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    glw_state->device->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
    glw_state->device->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
//	glw_state->device->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS ); // New
//	glw_state->device->SetRenderState( D3DRS_STENCILENABLE, FALSE ); // Stencil done in PS

    glw_state->device->SetRenderState( D3DRS_FOGENABLE, FALSE );
    glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    glw_state->device->SetRenderState( D3DRS_BLENDOP,   D3DBLENDOP_ADD );    
    glw_state->device->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
    glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	// Put our original back buffer in texture 0, put the old Z/Stencil in texture 1:
    glw_state->device->SetTexture( 0, pTextureSrc );
	glw_state->device->SetTexture( 1, (LPDIRECT3DTEXTURE8)&zBufferSurface );

    glw_state->device->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    glw_state->device->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
    glw_state->device->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

    // Pass texture coords without transformation
    glw_state->device->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );  
    glw_state->device->SetTextureStageState( 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );  

    // Each texture has different tex coords
    glw_state->device->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 ); 
    glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSU,
                                       D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSV,
                                       D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MAXMIPLEVEL, 0 );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_NONE );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_GAUSSIANCUBIC );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_GAUSSIANCUBIC );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLORKEYOP,
                                       D3DTCOLORKEYOP_DISABLE );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLORSIGN, 0 );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ALPHAKILL,
                                       D3DTALPHAKILL_DISABLE );

	// Z/Stencil is similar, but we don't want any filtering, just sampling:
    glw_state->device->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 0 ); 
    glw_state->device->SetTextureStageState( 1, D3DTSS_ADDRESSU,
                                       D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 1, D3DTSS_ADDRESSV,
                                       D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 1, D3DTSS_MAXMIPLEVEL, 0 );
    glw_state->device->SetTextureStageState( 1, D3DTSS_MIPFILTER, D3DTEXF_NONE );
    glw_state->device->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_POINT );
    glw_state->device->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_POINT );
    glw_state->device->SetTextureStageState( 1, D3DTSS_COLORKEYOP,
                                       D3DTCOLORKEYOP_DISABLE );
    glw_state->device->SetTextureStageState( 1, D3DTSS_COLORSIGN, 0 );
    glw_state->device->SetTextureStageState( 1, D3DTSS_ALPHAKILL,
                                       D3DTALPHAKILL_DISABLE );

    // Use extract hot pixel shader
    glw_state->device->SetPixelShader( m_dwExtractHotPixelShader );       

	float CutoffScale[8];
	CutoffScale[0] = CutoffScale[1] = CutoffScale[2] = CutoffScale[3] = r_hdrcutoff->value;
	CutoffScale[4] = CutoffScale[5] = CutoffScale[6] = CutoffScale[7] = r_hdrcutoff->value / (1.0f - r_hdrcutoff->value);
	glw_state->device->SetPixelShaderConstant( CP_EXTRACT_CUTOFF, CutoffScale, 2 );

    // For screen-space texture-mapped quadrilateral
    glw_state->device->SetVertexShader( D3DFVF_XYZRHW|D3DFVF_TEX1 );   

    // Prepare quadrilateral vertices
    float x0 = (float)pRectDst->left;
    float y0 = (float)pRectDst->top;
    float x1 = (float)pRectDst->right;
    float y1 = (float)pRectDst->bottom;
    struct Quad
    {
        float x, y, z, w1;
        struct uv
        {
            float u, v;
        }
        tex;  
    } 
    aQuad[4] =
    { //  X   Y     Z   1/W     u0  v0      u1  v1      u2  v2      u3  v3
        {x0, y0, 1.0f, 1.0f, }, // texture coords are set below
        {x1, y0, 1.0f, 1.0f, },
        {x0, y1, 1.0f, 1.0f, },
        {x1, y1, 1.0f, 1.0f, }
    };

    // Set rendering to just the destination rect
    glw_state->device->SetScissors( 1, FALSE, (D3DRECT *)pRectDst );

  
    // Draw a quad for each block of 4 filter coefficients
    float u0 = (float)pRectSrc->left;
    float v0 = (float)pRectSrc->top;
    float u1 = (float)pRectSrc->right;
    float v1 = (float)pRectSrc->bottom;


    if( XGIsSwizzledFormat( descSrc.Format ) )
    {
        float fWidthScale = 1.f / (float)descSrc.Width;
        float fHeightScale = 1.f / (float)descSrc.Height;
        u0 *= fWidthScale;
        v0 *= fHeightScale;
        u1 *= fWidthScale;
        v1 *= fHeightScale;
    }
    
    aQuad[0].tex.u = u0;
    aQuad[0].tex.v = v0;
    aQuad[1].tex.u = u1;
    aQuad[1].tex.v = v0;
    aQuad[2].tex.u = u0;
    aQuad[2].tex.v = v1;
    aQuad[3].tex.u = u1;
    aQuad[3].tex.v = v1;

    // Draw the quad
    glw_state->device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP,
                                   2, aQuad, sizeof(Quad) ); 

    glw_state->device->SetTexture( 0, NULL );
    glw_state->device->SetTexture( 1, NULL );

    // Restore render target and zbuffer
    glw_state->device->SetRenderTarget( pRenderTarget, pZBuffer );

	if( pRenderTarget )
		pRenderTarget->Release();

	if( pZBuffer )
		pZBuffer->Release();

    glw_state->device->SetScreenSpaceOffset( 0.0f, 0.0f );

	// Restore ztest?
//	glw_state->device->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
}


void VVHighDynamicRange::HotBlur()
{
    // Make D3DTexture wrapper around current render target
    LPDIRECT3DSURFACE8 pRenderTarget;
    glw_state->device->GetRenderTarget( &pRenderTarget );
    D3DSURFACE_DESC descRenderTarget;
    pRenderTarget->GetDesc( &descRenderTarget );
    D3DTexture RenderTargetTexture;
    ZeroMemory( &RenderTargetTexture, sizeof(RenderTargetTexture) );
    XGSetTextureHeader( descRenderTarget.Width, descRenderTarget.Height,
                        1, 0, descRenderTarget.Format, 0,
                        &RenderTargetTexture, pRenderTarget->Data,
                        descRenderTarget.Width * 4 );
    pRenderTarget->Release();
    
    // Filters align to blurriest point in supersamples, on the pixel centers
    //   This takes advantage of the bilinear filtering in the texture map lookup.
    FilterSample YFilter[] =        // 1221 4-tap filter in Y
    {
        { 2.0f/6.f,  0.0f,  1.0f },
        { 1.0f/6.f,  0.0f,  3.0f },
        { 2.0f/6.f,  0.0f, -1.0f },
        { 1.0f/6.f,  0.0f, -3.0f },
    };
    FilterSample XFilter[] =        // 1221 4-tap filter in X
    {
        { 2.0f/6.f,  1.0f, 0.0f },
        { 1.0f/6.f,  3.0f, 0.0f },
        { 2.0f/6.f, -1.0f, 0.0f },
        { 1.0f/6.f, -3.0f, 0.0f },
    };

    D3DTexture *pTextureSrc;
    D3DTexture *pTextureDst;

    pTextureDst = &RenderTargetTexture; // source is backbuffer 

    // extract "hot" portion of hte image with downsampling
    pTextureSrc = pTextureDst;
    pTextureDst = m_rpHotImage;    // destination is blur texture
    ExtractHot( pTextureDst, pTextureSrc, 2, 2 );
//    ExtractHot( pTextureDst, pTextureSrc, 1, 1 );

    // 2 passes: Vertical gaussian (1221) followed by
    // horizontal gaussian (1221), with 2x2 downsampling    
    pTextureSrc = pTextureDst;  // destination is next blur texture
    pTextureDst = m_rpBlur[0];    // destination is blur texture
    FilterCopy(pTextureDst, pTextureSrc, 4, YFilter, 1, 2, false);
        
    pTextureSrc = pTextureDst;  // source is previous blur texture
    pTextureDst = m_rpBlur[1];  // destination is next blur texture
    FilterCopy(pTextureDst, pTextureSrc, 4, XFilter, 2, 1, false);

    m_pBlur = pTextureDst;
}