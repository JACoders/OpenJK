//
//
// Win_HighDynamicRange.h
//
// Declaration of high dynamic range effect class
//
//

#ifndef _WIN_HIGHDYNAMICRANGE_H_
#define _WIN_HIGHDYNAMICRANGE_H_

struct FilterSample
{
    float fValue;               // coefficient
    float fOffsetX, fOffsetY;   // subpixel offsets of supersamples in
                                //   destination coordinates
};


class VVHighDynamicRange
{
    // The blur filters are multipass and need temporary space.
#define BLUR_COUNT 2
    D3DTexture *m_rpHotImage;           // hot image
    D3DTexture *m_rpBlur[BLUR_COUNT];   // bluring textures of decreasing size
    D3DTexture *m_pBlur;                // current blur texture, set by Blur()

    // Light blend intensity scale factor
    float m_fBloomScale;
  
    // Pixel shader handles
    DWORD m_dwHotBlurPixelShader;      // blur the hot image
    DWORD m_dwExtractHotPixelShader;   // extract hot image

	bool  m_bInitialized;
  
    // Filtering routine that draws the source texture multiple
    // times, with sub-pixel offsets and filter coefficients.
    void FilterCopy( LPDIRECT3DTEXTURE9 pTextureDst,
                        LPDIRECT3DTEXTURE9 pTextureSrc,
                        UINT nSample,
                        FilterSample rSample[],
                        UINT nSuperSampleX,
                        UINT nSuperSampleY,
                        bool bCrap,
                        RECT *pRectDst = NULL,  // The destination texture is
                                                //   written only within this
                                                //   region.
                        RECT *pRectSrc = NULL );// The source texture is read
                                                //   outside of this region by
                                                //   the halfwidth of the
                                                //   filter.

    // extract hot image with downsamping
    void ExtractHot( LPDIRECT3DTEXTURE9 pTextureDst,
                        LPDIRECT3DTEXTURE9 pTextureSrc,
                        UINT nSuperSampleX, UINT nSuperSampleY,
                        RECT* pRectDst = NULL,
                        RECT* pRectSrc = NULL );
                       
    // Blur hot texture and set m_pBlur.  Calls FilterCopy with
    // different filter coefficients and offsets
    void HotBlur();
    
    // Demonstrate the inputs to the full high dynamic range effect.
    void DrawHotBlur(); // draw blurred "hot" texture
    
public:
    virtual void Initialize();
    virtual void Render();

    VVHighDynamicRange();
};

extern VVHighDynamicRange HDREffect;

#endif