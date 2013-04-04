//
//
// win_lighteffects.cpp
//
// Various lighting effects w/ pixel shaders
//
//

#ifdef VV_LIGHTING

#include "../server/exe_headers.h"

#include "../renderer/tr_local.h"
#include "glw_win_dx8.h"
#include "win_local.h"
#include "../renderer/tr_lightmanager.h"

#include "win_lighteffects.h"

#include <xgraphics.h>
#include <xgmath.h>

#include "shader_constants.h"


LightEffects::LightEffects()
{
    m_pCubeMap          = NULL;
	m_pBumpMap			= NULL;
	m_pSpecularMap		= NULL;
	m_pFalloffMap		= NULL;
	m_dwVertexShaderLight = 0L;
	m_dwPixelShaderLight = 0L;
	m_dwVertexShaderSpecular_Dynamic = 0L;
	m_dwPixelShaderSpecular_Dynamic = 0L;
	m_dwVertexShaderSpecular_Static = 0L;
	m_dwPixelShaderSpecular_Static = 0L;
	m_dwVertexShaderEnvironment = 0L;
	m_dwVertexShaderBump = 0L;
	m_dwPixelShaderBump = 0L;
	m_bInLightPhase	    = false;
	m_bInitialized      = false;

	Initialize();
}


LightEffects::~LightEffects()
{
    if( m_pCubeMap )
		m_pCubeMap->Release();
	m_pCubeMap = NULL;

	if(m_pBumpMap)
		m_pBumpMap->Release();
	m_pBumpMap = NULL;

	if(m_pFalloffMap)
		m_pFalloffMap->Release();
	m_pFalloffMap = NULL;
    
	if(m_pSpecularMap)
		m_pSpecularMap->Release();
	m_pSpecularMap = NULL;

    if( glw_state->device )
	{
        glw_state->device->DeleteVertexShader( m_dwVertexShaderLight );
		glw_state->device->DeletePixelShader( m_dwPixelShaderLight );
		glw_state->device->DeleteVertexShader( m_dwVertexShaderSpecular_Dynamic );
		glw_state->device->DeletePixelShader( m_dwPixelShaderSpecular_Dynamic );
		glw_state->device->DeleteVertexShader( m_dwVertexShaderSpecular_Static );
		glw_state->device->DeletePixelShader( m_dwPixelShaderSpecular_Static );
		glw_state->device->DeleteVertexShader( m_dwVertexShaderEnvironment );
		glw_state->device->DeletePixelShader( m_dwPixelShaderBump );
		glw_state->device->DeleteVertexShader( m_dwVertexShaderBump );
	}
}


void LightEffects::StartLightPhase()
{
	m_bInLightPhase = true;
}


void LightEffects::EndLightPhase()
{
	m_bInLightPhase = false;
}

extern const char *Sys_RemapPath( const char *filename );

bool LightEffects::Initialize()
{
	HRESULT hr;

	// Create a vertex shader
    DWORD dwVertexDecl[] =
    {
        D3DVSD_STREAM( 0 ),
        D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),     // v0 = Position
		D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),	 // v1 = Normal
        D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),     // v2 = Base tex coords
        D3DVSD_REG( 3, D3DVSDT_FLOAT3 ),     // v3 = Tangent space tangent
        D3DVSD_END()
    };

    if(!( CreateVertexShader(Sys_RemapPath("base\\media\\dlight.xvu"), dwVertexDecl, &m_dwVertexShaderLight)))
		return false;

	if(!( CreateVertexShader(Sys_RemapPath("base\\media\\specular_dynamic.xvu"), dwVertexDecl, &m_dwVertexShaderSpecular_Dynamic)))
		return false;

	if(!( CreateVertexShader(Sys_RemapPath("base\\media\\specular_static.xvu"), dwVertexDecl, &m_dwVertexShaderSpecular_Static)))
		return false;

	DWORD dwVertexDeclBump[] =
	{ 
		D3DVSD_STREAM( 0 ),
		D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),     // v0 = Position
		D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),	 // v1 = Normal
		D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),     // v2 = Base tex coords 0
		D3DVSD_REG( 3, D3DVSDT_FLOAT2 ),     // v2 = Base tex coords 1
		D3DVSD_REG( 4, D3DVSDT_FLOAT3 ),     // v4 = Tangent space tangent
		D3DVSD_END()
	};

	if(!( CreateVertexShader(Sys_RemapPath("base\\media\\bump.xvu"), dwVertexDeclBump, &m_dwVertexShaderBump)))
		return false;

	DWORD dwVertexDeclEnv[] =
    {
        D3DVSD_STREAM( 0 ),
        D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),     // v0 = Position
		D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),	 // v1 = Normal
		D3DVSD_REG( 2, D3DVSDT_D3DCOLOR ),   // v2 = Color
		D3DVSD_END()
    };

	if(!( CreateVertexShader(Sys_RemapPath("base\\media\\environment.xvu"), dwVertexDeclEnv, &m_dwVertexShaderEnvironment)))
		return false;

	// Create the pixel shader
	if(!(CreatePixelShader(Sys_RemapPath("base\\media\\dlight.xpu"), &m_dwPixelShaderLight)))
		return false;

	if(!(CreatePixelShader(Sys_RemapPath("base\\media\\specular_dynamic.xpu"), &m_dwPixelShaderSpecular_Dynamic)))
		return false;

	if(!(CreatePixelShader(Sys_RemapPath("base\\media\\specular_static.xpu"), &m_dwPixelShaderSpecular_Static)))
		return false;

	if(!(CreatePixelShader(Sys_RemapPath("base\\media\\bump.xpu"), &m_dwPixelShaderBump)))
		return false;

	hr = D3DXCreateTextureFromFileEx(glw_state->device, 
		Sys_RemapPath("base\\media\\defaultbump.dds"),
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		0,
		D3DFMT_A8R8G8B8,
		0,
		D3DX_FILTER_LINEAR,
		D3DX_FILTER_LINEAR,
		0,
		NULL,
		NULL,
		&m_pBumpMap);
	if (FAILED(hr))
	{
		return false;
	}

	hr = D3DXCreateTextureFromFileEx(glw_state->device, 
		Sys_RemapPath("base\\media\\diffspec.dds"),
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		0,
		D3DFMT_A8R8G8B8,
		0,
		D3DX_FILTER_LINEAR,
		D3DX_FILTER_LINEAR,
		0,
		NULL, 
		NULL, 
		&m_pSpecularMap);
	if (FAILED(hr))
	{
		return false;
	}

	// Create the volume falloff texture
	UINT width = 32, 
		 height = 32, 
		 depth = 32, 
		 levels = 1;
	D3DFORMAT format = D3DFMT_A8;
	if (FAILED (hr = glw_state->device->CreateVolumeTexture(width, height, depth,
		1, 0, format, D3DPOOL_DEFAULT, &m_pFalloffMap) ) )
	{
		return false;
	}

	// Fill the volume texture
    D3DVOLUME_DESC  desc;
    D3DLOCKED_BOX   lock;
    m_pFalloffMap->GetLevelDesc( 0, &desc );
    m_pFalloffMap->LockBox( 0, &lock, 0, 0L );
    BYTE* pBits = (BYTE*)lock.pBits;

    for( UINT w=0; w<width; w++ )
    {
        for( UINT v=0; v<height; v++ )
        {
            for( UINT u=0; u<depth; u++ )
            {
				FLOAT x = (2.0f*u)/(width-1) - 1.0f; // Ranges from -1 to +1
				FLOAT y = (2.0f*v)/(height-1) - 1.0f; // Ranges from -1 to +1
				FLOAT z = (2.0f*w)/(depth-1) - 1.0f;

				FLOAT distance = (float)(x*x + y*y + z*z);
				if (distance == 0) 
				{
					*pBits++ = (BYTE)255;
				}
				else
				{
                    FLOAT falloff = min(1.0f, max(0.0f, ((1.0f/2.0f)/distance - 1.0f/2.0f)/1.0f ) );
					*pBits++ = (BYTE)(255*falloff);
				}
            }
        }
    }

	DWORD dwPixelSize   = XGBytesPerPixelFromFormat( desc.Format );
    DWORD dwTextureSize = desc.Width * desc.Height * desc.Depth * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, lock.pBits, dwTextureSize );
    
    XGSwizzleBox( pSrcBits, 0, 0, NULL, lock.pBits, 
                  desc.Width, desc.Height, desc.Depth, 
                  NULL, dwPixelSize );

    delete [] pSrcBits;

	m_pFalloffMap->UnlockBox( 0 );

	// Create the normalization cube map
    if(!CreateNormalizationCubeMap( 64, &m_pCubeMap ))
        return false;

	m_bInitialized = true;

    return true;
}


bool LightEffects::RenderDynamicLights()
{
	VVdlight_t *dl;
	shaderStage_t *dStage;
	vec3_t	origin;
	byte	clipBits[SHADER_MAX_VERTEXES];
	glIndex_t	hitIndexes[SHADER_MAX_INDEXES];
	int		numIndexes;
	float	radius;
	int		fogging;
	vec3_t	dist;
	vec3_t	e1;
	vec3_t	e2;
	vec3_t	normal;
	float	fac;

	if(!VVLightMan.num_dlights)
		return true;
	
	glw_state->device->SetRenderState( D3DRS_LIGHTING,         FALSE );   
    
	glw_state->device->SetTextureStageState( 1, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP );
	glw_state->device->SetTextureStageState( 1, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP );
    glw_state->device->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR );

	glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSW,  D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

	glw_state->device->SetTextureStageState( 3, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 3, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 3, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 3, D3DTSS_MIPFILTER, D3DTEXF_POINT );

	glw_state->device->SetTextureStageState( 2, D3DTSS_ALPHAKILL, D3DTALPHAKILL_ENABLE);
	
//	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	if(tess.shader->isBumpMap)
	{
		for ( int stage = 0; stage < tess.shader->numUnfoggedPasses; stage++ )
		{
			shaderStage_t *pStage = &tess.xstages[stage];
			if(pStage->isBumpMap)
			{
				glwstate_t::texturexlat_t::iterator i = glw_state->textureXlat.find(pStage->bundle[1].image->texnum);
				glw_state->device->SetTexture( 1, i->second.mipmap );
			}
		}
	}
	else
        glw_state->device->SetTexture(1, m_pBumpMap); 
	
	glw_state->device->SetTexture(2, m_pFalloffMap);
	glw_state->device->SetTexture(3, m_pCubeMap);
	
	glw_state->device->SetPixelShader(m_dwPixelShaderLight);
	glw_state->device->SetVertexShader(m_dwVertexShaderLight);

	for(int l = 0; l < VVLightMan.num_dlights; l++)
	{  
		if(!(tess.dlightBits & (1 << l)))  
			continue;

		dl = &VVLightMan.dlights[l];
		if(!dl)
			continue;

		// Not going to bother testing all the polygons in a ghoul2 model
		// they almost always all end up in range anyway
		if(!backEnd.currentEntity->e.ghoul2)
		{
            VectorCopy( dl->transformed, origin );
			radius = dl->radius;

			int		clipall = 63;
			for (int i = 0 ; i < tess.numVertexes ; i++) 
			{
				int		clip;
				VectorSubtract( origin, tess.xyz[i], dist );

				clip = 0;
				if (  dist[0] < -radius ) 
				{
					clip |= 1;
				}
				else if ( dist[0] > radius ) 
				{
					clip |= 2;
				}
				if (  dist[1] < -radius ) 
				{
					clip |= 4;
				}
				else if ( dist[1] > radius ) 
				{
					clip |= 8;
				}
				if (  dist[2] < -radius ) 
				{
					clip |= 16;
				}
				else if ( dist[2] > radius ) 
				{
					clip |= 32;
				}

				clipBits[i] = clip;
				clipall &= clip;
			}
			if ( clipall ) 
			{
				continue;	// this surface doesn't have any of this light
			}
		
			// build a list of triangles that need light
			numIndexes = 0;
			for ( i = 0 ; i < tess.numIndexes ; i += 3 ) 
			{
				int		a, b, c;

				a = tess.indexes[i];
				b = tess.indexes[i+1];
				c = tess.indexes[i+2];
				if ( clipBits[a] & clipBits[b] & clipBits[c] ) 
				{
					continue;	// not lighted
				}

				VectorSubtract( tess.xyz[a], tess.xyz[b], e1);
				VectorSubtract( tess.xyz[c], tess.xyz[b], e2);
				CrossProduct(e1,e2,normal);

				VectorNormalize(normal);
				fac=DotProduct(normal,origin)-DotProduct(normal, tess.xyz[a]);
				if (fac >= radius)  // out of range
				{
					continue;
				}
				
				// save the indexes
				hitIndexes[numIndexes] = tess.indexes[i];
				hitIndexes[numIndexes + 1] = tess.indexes[i + 1];
				hitIndexes[numIndexes + 2] = tess.indexes[i + 2];

				numIndexes += 3;

				if (numIndexes>=SHADER_MAX_VERTEXES-3)
				{
					break; // we are out of space, so we are done :)
				}
			}

			if ( !numIndexes ) {
				continue;
			}
		}

		//don't have fog enabled when we redraw with alpha test, or it will double over
		//and screw the tri up -rww
		if (r_drawfog->value == 2 && 
			tr.world &&
			(tess.fogNum == tr.world->globalFog || tess.fogNum == tr.world->numfogs))
		{
			fogging = qglIsEnabled(GL_FOG);

			if (fogging)
			{
				qglDisable(GL_FOG);
			}
		}
		else
		{
			fogging = 0;
		}

		dStage = NULL;
		if (tess.shader && qglActiveTextureARB)
		{
			int i = 0;
			while (i < tess.shader->numUnfoggedPasses)
			{
				const int blendBits = (GLS_SRCBLEND_BITS+GLS_DSTBLEND_BITS);
				if (((tess.shader->stages[i].bundle[0].image && !tess.shader->stages[i].bundle[0].isLightmap && !tess.shader->stages[i].bundle[0].numTexMods && tess.shader->stages[i].bundle[0].tcGen != TCGEN_ENVIRONMENT_MAPPED && tess.shader->stages[i].bundle[0].tcGen != TCGEN_FOG) ||
					(tess.shader->stages[i].bundle[1].image && !tess.shader->stages[i].bundle[1].isLightmap && !tess.shader->stages[i].bundle[1].numTexMods && tess.shader->stages[i].bundle[1].tcGen != TCGEN_ENVIRONMENT_MAPPED && tess.shader->stages[i].bundle[1].tcGen != TCGEN_FOG)) &&
					(tess.shader->stages[i].stateBits & blendBits) == 0 )
				{ //only use non-lightmap opaque stages
					dStage = &tess.shader->stages[i];
					break;
				}
				i++;
			}
		}

		if (dStage)
		{
			GL_SelectTexture( 0 );
			GL_State(0);
			// animMaps don't actually have a texture in image, it's an array of image pointers:
			if (dStage->bundle[0].numImageAnimations > 1)
			{
				int index;

				if (backEnd.currentEntity->e.renderfx & RF_SETANIMINDEX )
				{
					index = backEnd.currentEntity->e.skinNum;
				}
				else
				{
					// it is necessary to do this messy calc to make sure animations line up
					// exactly with waveforms of the same frequency
					index = myftol( backEnd.refdef.floatTime * dStage->bundle[0].imageAnimationSpeed * FUNCTABLE_SIZE );
					index >>= FUNCTABLE_SIZE2;
					
					if ( index < 0 ) {
						index = 0;	// may happen with shader time offsets
					}
				}

				if ( dStage->bundle[0].oneShotAnimMap )
				{
					if ( index >= dStage->bundle[0].numImageAnimations )
					{
						// stick on last frame
						index = dStage->bundle[0].numImageAnimations - 1;
					}
				}
				else
				{
					// loop
					index %= dStage->bundle[0].numImageAnimations;
				}

				GL_Bind( *((image_t**)dStage->bundle[0].image + index) );
			}
			else if (dStage->bundle[0].image && !dStage->bundle[0].isLightmap && !dStage->bundle[0].numTexMods && dStage->bundle[0].tcGen != TCGEN_ENVIRONMENT_MAPPED && dStage->bundle[0].tcGen != TCGEN_FOG)
			{
				GL_Bind( dStage->bundle[0].image );
			}
			else
			{
				GL_Bind( dStage->bundle[1].image );
			}

			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);// | GLS_DEPTHFUNC_EQUAL);
		}
		else
		{
			GL_Bind( tr.whiteImage );
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE);// | GLS_DEPTHFUNC_EQUAL );
		}

		glwstate_t::texturexlat_t::iterator inf = glw_state->textureXlat.find(glw_state->currentTexture[0]);

		glw_state->device->SetTexture(0, inf->second.mipmap);

		D3DXVECTOR4 vecLightRange(1.0f / dl->radius, 0.0f, 0.0f, 0.0f);
		glw_state->device->SetVertexShaderConstant(CV_ONE_OVER_LIGHT_RANGE, (void*)&vecLightRange.x, 1);

		glw_state->device->SetPixelShaderConstant(CP_DIFFUSE_COLOR, &dl->color[0], 1);

		ProcessVertices( (D3DXVECTOR3*)&dl->direction, (D3DXVECTOR3*)&dl->transformed );

		if(backEnd.currentEntity->e.ghoul2)
			renderObject_Light( tess.numIndexes, tess.indexes );
		else
            renderObject_Light( numIndexes, hitIndexes );

		if (fogging)
		{
			qglEnable(GL_FOG);
		}
	}

	glw_state->device->SetPixelShader( 0 );

	// This is kinda a hack to get the quake renderer to discard
	// these textures after they are used here, instead of applying
	// them to more geometry
	glState.currenttextures[0] = -2;
	glState.currenttextures[1] = -2;
	glw_state->currentTexture[0] = -2;
	glw_state->currentTexture[1] = -2;

	glw_state->device->SetTextureStageState( 2, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE);

	glw_state->device->SetTexture(2, NULL);
	glw_state->device->SetTexture(3, NULL);
	
	return true;
}


void LightEffects::RenderSpecular()
{
	glw_state->device->SetRenderState( D3DRS_LIGHTING,         FALSE );   
	glw_state->device->SetRenderState( D3DRS_FOGENABLE,		   FALSE );

	glwstate_t::texturexlat_t::iterator i = glw_state->textureXlat.find(glw_state->currentTexture[0]);

	glw_state->device->SetTexture( 0, i->second.mipmap );   
	glw_state->device->SetTexture( 2, m_pSpecularMap );

	glw_state->device->SetVertexShaderConstant(CV_CAMERA_DIRECTION, D3DXVECTOR4(tr.viewParms.or.axis[0][0],
		tr.viewParms.or.axis[0][1],
		tr.viewParms.or.axis[0][2],
		1.0f), 1 );

	RenderSpecular_Static();

	if(tess.dlightBits)
		RenderSpecular_Dynamic();

	glw_state->device->SetPixelShader(0);

	// This is kinda a hack to get the quake renderer to discard
	// these textures after they are used here, instead of applying
	// them to more geometry
	glState.currenttextures[0] = -2;
	glState.currenttextures[1] = -2;
	glw_state->currentTexture[0] = -2;
	glw_state->currentTexture[1] = -2;
}


bool LightEffects::RenderSpecular_Dynamic()
{
	VVdlight_t *dl;

	glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP );
	glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	glw_state->device->SetTextureStageState( 2, D3DTSS_MIPFILTER, D3DTEXF_POINT );
	glw_state->device->SetTextureStageState( 3, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 3, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 3, D3DTSS_ADDRESSW,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 3, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

	glw_state->device->SetTexture( 3, m_pFalloffMap );

	glw_state->device->SetPixelShader(m_dwPixelShaderSpecular_Dynamic);
	glw_state->device->SetVertexShader(m_dwVertexShaderSpecular_Dynamic);

	for(int i = 0; i < VVLightMan.num_dlights; i++)
	{  
		if(!(tess.dlightBits & (1 << i)))  
			continue;

		dl = &VVLightMan.dlights[i];

		D3DXVECTOR4 vecLightRange(1.0f / dl->radius, 0.0f, 0.0f, 0.0f);
		glw_state->device->SetVertexShaderConstant(CV_ONE_OVER_LIGHT_RANGE, (void*)&vecLightRange.x, 1);

		ProcessVertices( (D3DXVECTOR3*)&dl->direction, (D3DXVECTOR3*)&dl->transformed );

		renderObject_Light( tess.numIndexes, tess.indexes);
	}

	glw_state->device->SetTexture( 3, NULL );

	return true;
}


bool LightEffects::RenderSpecular_Static()
{
	glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP );
	glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP );
	glw_state->device->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 2, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	glw_state->device->SetTextureStageState( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
	glw_state->device->SetTextureStageState( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	glw_state->device->SetTextureStageState( 2, D3DTSS_MIPFILTER, D3DTEXF_NONE );

	glw_state->device->SetPixelShader(m_dwPixelShaderSpecular_Static);
	glw_state->device->SetVertexShader(m_dwVertexShaderSpecular_Static);

	ProcessVertices( NULL, NULL );

	D3DXVECTOR4 vLight;
	if (backEnd.currentEntity && (backEnd.currentEntity->e.hModel||backEnd.currentEntity->e.ghoul2) )
	{
		vLight.x = backEnd.currentEntity->lightDir[0];
		vLight.y = backEnd.currentEntity->lightDir[1];
		vLight.z = backEnd.currentEntity->lightDir[2];
	}
	else
	{
		// These values were taken from the RB_CalcSpecularAlpha default case
		vLight.x = -960.0f;
		vLight.y = 1920.0f;
		vLight.z = 96.0f;
	}
	
	vLight.w = 1.0f;

	glw_state->device->SetVertexShaderConstant(CV_LIGHT_DIRECTION, vLight, 1);

	renderObject_Light( tess.numIndexes, tess.indexes );

	return true;
}

bool LightEffects::RenderEnvironment()
{
	glw_state->device->SetRenderState( D3DRS_LIGHTING, false );

	glw_state->device->SetVertexShaderConstant(CV_CAMERA_DIRECTION, D3DXVECTOR4(backEnd.ori.viewOrigin[0],
																				backEnd.ori.viewOrigin[1],
																				backEnd.ori.viewOrigin[2],					
																				1.0f), 1 );

	ProcessVertices(NULL, NULL);

	XGMATRIX *view, viewtran;
	view = (XGMATRIX*)glw_state->matrixStack[glw_state->MatrixMode_Model]->GetTop();
	XGMatrixTranspose( &viewtran, view );
	glw_state->device->SetVertexShaderConstant(CV_VIEW_0, viewtran, 4);

	glw_state->device->SetVertexShader(m_dwVertexShaderEnvironment);

	renderObject_Env();

	return true;
}


// Renders bump maps without the benefit of dynamic lights
void LightEffects::RenderBump()
{
	glw_state->device->SetRenderState( D3DRS_LIGHTING, false );
	glw_state->device->SetRenderState( D3DRS_FOGENABLE, false );

	glwstate_t::texturexlat_t::iterator i = glw_state->textureXlat.find(glw_state->currentTexture[0]);
	glw_state->device->SetTexture( 0, i->second.mipmap );

	i = glw_state->textureXlat.find(glw_state->currentTexture[1]);
	glw_state->device->SetTexture( 1, i->second.mipmap );

	glw_state->device->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, i->second.anisotropy);
	glw_state->device->SetTextureStageState(0, D3DTSS_MINFILTER, i->second.minFilter);
	glw_state->device->SetTextureStageState(0, D3DTSS_MIPFILTER, i->second.mipFilter);
	glw_state->device->SetTextureStageState(0, D3DTSS_MAGFILTER, i->second.magFilter);
	glw_state->device->SetTextureStageState(0, D3DTSS_ADDRESSU, i->second.wrapU);
	glw_state->device->SetTextureStageState(0, D3DTSS_ADDRESSV, i->second.wrapV);

	glw_state->device->SetTextureStageState(1, D3DTSS_MAXANISOTROPY, i->second.anisotropy);
	glw_state->device->SetTextureStageState(1, D3DTSS_MINFILTER, i->second.minFilter);
	glw_state->device->SetTextureStageState(1, D3DTSS_MIPFILTER, i->second.mipFilter);
	glw_state->device->SetTextureStageState(1, D3DTSS_MAGFILTER, i->second.magFilter);
	glw_state->device->SetTextureStageState(1, D3DTSS_ADDRESSU, i->second.wrapU);
	glw_state->device->SetTextureStageState(1, D3DTSS_ADDRESSV, i->second.wrapV);

	glw_state->device->SetRenderState(D3DRS_SPECULARENABLE, true);

	glw_state->device->SetPixelShader(m_dwPixelShaderBump);
	glw_state->device->SetVertexShader(m_dwVertexShaderBump);

	ProcessVertices( NULL, NULL );

	D3DXVECTOR4 vAmbient, vDiffuse, vLightDir;

	if (backEnd.currentEntity && (backEnd.currentEntity->e.hModel||backEnd.currentEntity->e.ghoul2) )
	{
		if(tess.shader->stages[tess.currentPass].rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
		{
			vAmbient.x = (backEnd.currentEntity->ambientLight[0] / 255.f) * (backEnd.currentEntity->e.shaderRGBA[0] / 255.0);
			vAmbient.y = (backEnd.currentEntity->ambientLight[1] / 255.f) * (backEnd.currentEntity->e.shaderRGBA[1] / 255.0);
			vAmbient.z = (backEnd.currentEntity->ambientLight[2] / 255.f) * (backEnd.currentEntity->e.shaderRGBA[2] / 255.0);
		}
		else
		{
			vAmbient.x = backEnd.currentEntity->ambientLight[0] / 255.f;
			vAmbient.y = backEnd.currentEntity->ambientLight[1] / 255.f;
			vAmbient.z = backEnd.currentEntity->ambientLight[2] / 255.f;
		}

		vDiffuse.x = backEnd.currentEntity->directedLight[0] / 255.f;
		vDiffuse.y = backEnd.currentEntity->directedLight[1] / 255.f;
		vDiffuse.z = backEnd.currentEntity->directedLight[2] / 255.f; 

		vLightDir.x = DotProduct( backEnd.currentEntity->lightDir, backEnd.currentEntity->e.axis[0] );
		vLightDir.y = DotProduct( backEnd.currentEntity->lightDir, backEnd.currentEntity->e.axis[1] );
		vLightDir.z = DotProduct( backEnd.currentEntity->lightDir, backEnd.currentEntity->e.axis[2] );
	} 
	else
	{
		vec3_t sundir;
		sundir[0] = r_sundir_x->value;
		sundir[1] = r_sundir_y->value;
		sundir[2] = r_sundir_z->value;

		VectorNormalize(sundir);

		vLightDir.x = sundir[0];//tr.sunDirection[0];
		vLightDir.y = sundir[1];//tr.sunDirection[1];
		vLightDir.z = sundir[2];//tr.sunDirection[2];
		vLightDir.w = 1.0f;

		vAmbient.x = tr.sunAmbient[0] / 1.5f;
		vAmbient.y = tr.sunAmbient[1] / 1.5f;
		vAmbient.z = tr.sunAmbient[2] / 1.5f;

		vDiffuse.x = 1.0f;
		vDiffuse.y = 1.0f;
		vDiffuse.z = 1.0f;	
	}

	glw_state->device->SetPixelShaderConstant(CP_AMBIENT_COLOR, vAmbient, 1);
	glw_state->device->SetPixelShaderConstant(CP_DIFFUSE_COLOR, vDiffuse, 1);
	
	glw_state->device->SetVertexShaderConstant(CV_LIGHT_DIRECTION, vLightDir, 1);

	glw_state->device->SetVertexShaderConstant(CV_CAMERA_DIRECTION, D3DXVECTOR4(tr.viewParms.or.axis[0][0],
		tr.viewParms.or.axis[0][1],
		tr.viewParms.or.axis[0][2],
		1.0f), 1 );

	renderObject_Bump();

	glw_state->device->SetPixelShader( 0 );

	glw_state->device->SetRenderState(D3DRS_SPECULARENABLE, false);
}


//bool LightEffects::RenderStaticLights()
//{
//	VVslight_t *sl;
//
//	for(int i = 0; i < tess.numSlights; i++)
//	{  
//		sl = &VVLightMan.slights[tess.slightBits[i]];
//
//		D3DXVECTOR4 vecLightRange(1.0f / (sl->radius * 2.0f), sl->radius, 1.0f, 1.0f);
//		glw_state->device->SetVertexShaderConstant(CV_ONE_OVER_LIGHT_RANGE, (void*)&vecLightRange.x, 1);
//
//		glw_state->device->SetVertexShaderConstant(CV_LIGHT_COLOR, &sl->color[0], 1);
//
//		ProcessVertices( NULL, (D3DXVECTOR3*)&sl->origin );
//
//		renderObject_Light();
//
//		tess.currentPass++;
//	}
//
//    return true;
//}


void LightEffects::ProcessVertices( D3DXVECTOR3* pDirLightDir, D3DXVECTOR3* pPtLightPos )
{
	// Just in case, this doesn't always get set
	glw_state->device->SetTransform( D3DTS_PROJECTION, glw_state->matrixStack[glw_state->MatrixMode_Projection]->GetTop() );

	// Compute the matrix set
    XGMATRIX matComposite, matProjectionViewport, matWorld;
	// Get the projection viewport matrix the fixed pipeline uses.
	// The viewport matrix includes the viewport x,y scale and the 
	// appropriate z scale.
	glw_state->device->GetProjectionViewportMatrix( &matProjectionViewport );

	D3DVIEWPORT8 view;
	glw_state->device->GetViewport(&view);

	// Gotta do this to fix an XDK bug
	// GetProjectionViewportMatrix does not seem to reflect the viewport values
	// when the viewport is offset
	matProjectionViewport._31 += view.X;
	matProjectionViewport._32 += view.Y;

	XGMatrixMultiply( &matComposite, (XGMATRIX*)glw_state->matrixStack[glwstate_t::MatrixMode_Model]->GetTop(), &matProjectionViewport );

	// Transpose and set the composite matrix.
	XGMatrixTranspose( &matComposite, &matComposite );
	glw_state->device->SetVertexShaderConstant( CV_WORLDVIEWPROJ_0, &matComposite, 4 );

	if (pPtLightPos)
		glw_state->device->SetVertexShaderConstant( CV_LIGHT_POSITION, pPtLightPos,  1 );

	// Set viewport offsets.
	float fViewportOffsets[4] = { 0.53125f, 0.53125f, 0.0f, 0.0f };
	glw_state->device->SetVertexShaderConstant( CV_VIEWPORT_OFFSETS, &fViewportOffsets, 1 );

	// Set common constants
	glw_state->device->SetVertexShaderConstant(CV_ONE, D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f), 1);
	glw_state->device->SetVertexShaderConstant(CV_HALF, D3DXVECTOR4(0.5f, 0.5f, 0.5f, 0.5f), 1);
}


inline D3DCOLOR VectorToRGBA( const D3DXVECTOR3* v, FLOAT fHeight = 1.0f )
{
    D3DCOLOR r = (D3DCOLOR)( ( v->x + 1.0f ) * 127.5f );
    D3DCOLOR g = (D3DCOLOR)( ( v->y + 1.0f ) * 127.5f );
    D3DCOLOR b = (D3DCOLOR)( ( v->z + 1.0f ) * 127.5f );
    D3DCOLOR a = (D3DCOLOR)( 255.0f * fHeight );
    return( (a<<24L) + (r<<16L) + (g<<8L) + (b<<0L) );
}


bool LightEffects::CreateNormalizationCubeMap( DWORD dwSize, LPDIRECT3DCUBETEXTURE8* ppCubeMap )
{
    HRESULT hr;

    // Create the cube map
    if( FAILED( hr = glw_state->device->CreateCubeTexture( dwSize, 1, 0, D3DFMT_X8R8G8B8, 
                                                    D3DPOOL_DEFAULT, ppCubeMap ) ) )
        return false;
    
    // Allocate temp space for swizzling the cubemap surfaces
    DWORD* pSourceBits = new DWORD[ dwSize * dwSize ];

    // Fill all six sides of the cubemap
    for( DWORD i=0; i<6; i++ )
    {
        // Lock the i'th cubemap surface
        LPDIRECT3DSURFACE8 pCubeMapFace;
        (*ppCubeMap)->GetCubeMapSurface( (D3DCUBEMAP_FACES)i, 0, &pCubeMapFace );

        // Write the RGBA-encoded normals to the surface pixels
        DWORD*      pPixel = pSourceBits;
        D3DXVECTOR3 n;
        FLOAT       w, h;

        for( DWORD y = 0; y < dwSize; y++ )
        {
            h  = (FLOAT)y / (FLOAT)(dwSize-1);  // 0 to 1
            h  = ( h * 2.0f ) - 1.0f;           // -1 to 1
            
            for( DWORD x = 0; x < dwSize; x++ )
            {
                w = (FLOAT)x / (FLOAT)(dwSize-1);   // 0 to 1
                w = ( w * 2.0f ) - 1.0f;            // -1 to 1

                // Calc the normal for this texel
                switch( i )
                {
                    case D3DCUBEMAP_FACE_POSITIVE_X:    // +x
                        n.x = +1.0;
                        n.y = -h;
                        n.z = -w;
                        break;
                        
                    case D3DCUBEMAP_FACE_NEGATIVE_X:    // -x
                        n.x = -1.0;
                        n.y = -h;
                        n.z = +w;
                        break;
                        
                    case D3DCUBEMAP_FACE_POSITIVE_Y:    // y
                        n.x = +w;
                        n.y = +1.0;
                        n.z = +h;
                        break;
                        
                    case D3DCUBEMAP_FACE_NEGATIVE_Y:    // -y
                        n.x = +w;
                        n.y = -1.0;
                        n.z = -h;
                        break;
                        
                    case D3DCUBEMAP_FACE_POSITIVE_Z:    // +z
                        n.x = +w;
                        n.y = -h;
                        n.z = +1.0;
                        break;
                        
                    case D3DCUBEMAP_FACE_NEGATIVE_Z:    // -z
                        n.x = -w;
                        n.y = -h;
                        n.z = -1.0;
                        break;
                }

                // Store the normal as an RGBA color
                D3DXVec3Normalize( &n, &n );
                *pPixel++ = VectorToRGBA( &n );
            }
        }
        
        // Swizzle the result into the cubemap face surface
        D3DLOCKED_RECT lock;
        pCubeMapFace->LockRect( &lock, 0, 0L );
        XGSwizzleRect( pSourceBits, 0, NULL, lock.pBits, dwSize, dwSize,
                       NULL, sizeof(DWORD) );
        pCubeMapFace->UnlockRect();

        // Release the cubemap face
        pCubeMapFace->Release();
    }

    // Free temp space
    if( pSourceBits )
		delete [] pSourceBits;
	pSourceBits = NULL;

    return true;
}


#endif // VV_LIGHTING