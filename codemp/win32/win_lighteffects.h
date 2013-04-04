//
//
// win_lightefects.h
//
// Declaration of class for pixel shader light effects
//
//


#ifndef _WIN_LIGHTEFFECTS_H_
#define _WIN_LIGHTEFFECTS_H_


class LightEffects
{
public:
    LPDIRECT3DCUBETEXTURE8   m_pCubeMap;          // Normalization cubemap
	LPDIRECT3DTEXTURE8		 m_pBumpMap;
//	LPDIRECT3DTEXTURE8		 m_pSpecularMap;
	LPDIRECT3DVOLUMETEXTURE8 m_pFalloffMap;
	DWORD                    m_dwVertexShaderLight;    
	DWORD					 m_dwPixelShaderLight;
	/*DWORD					 m_dwVertexShaderSpecular_Dynamic;
	DWORD					 m_dwPixelShaderSpecular_Dynamic;
	DWORD					 m_dwVertexShaderSpecular_Static;
	DWORD					 m_dwPixelShaderSpecular_Static;*/
	DWORD					 m_dwVertexShaderEnvironment;
	DWORD					 m_dwVertexShaderBump;
	DWORD					 m_dwPixelShaderBump;
	bool					 m_bInLightPhase;
	bool					 m_bInitialized;

public:
    LightEffects();
    virtual ~LightEffects();

    bool Initialize();
	void ProcessVertices(D3DXVECTOR3* pPtLightPos);
    bool RenderDynamicLights();
	/*bool RenderStaticLights();
	void RenderSpecular();
	bool RenderSpecular_Dynamic();
	bool RenderSpecular_Static();*/
	bool RenderEnvironment();
	void RenderBump();
	bool CreateNormalizationCubeMap( DWORD dwSize, LPDIRECT3DCUBETEXTURE8* ppCubeMap );
	void StartLightPhase();
	void EndLightPhase();
};


#endif