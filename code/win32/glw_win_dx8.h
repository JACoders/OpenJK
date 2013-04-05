
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

#ifndef __GLW_WIN_H__
#define __GLW_WIN_H__

#include <map>

#include <d3d8.h>
#ifdef _WIN32
#include <d3dx8.h>
#endif 

#include "../renderer/qgl_console.h"
#include "../game/q_shared.h"
#include "../qcommon/qfiles.h"

#define GLW_MAX_TEXTURE_STAGES 2
#define GLW_MAX_STRIPS 2048 


struct glwstate_t
{
	// Interface to DX
	IDirect3DDevice8* device;

	// Matrix stuff
	enum MatrixMode
	{
		MatrixMode_Model = 0,
		MatrixMode_Projection = 1,
		MatrixMode_Texture0 = 2,
		MatrixMode_Texture1 = 3,
		MatrixMode_Texture2 = 4,
		MatrixMode_Texture3 = 5,

		Num_MatrixModes
	};
	
	ID3DXMatrixStack* matrixStack[Num_MatrixModes];
	MatrixMode matrixMode;

	// Current primitive mode (triangles/quads/strips)
	D3DPRIMITIVETYPE primitiveMode;

	// Are we in a glBegin/glEnd block? (Used for sanity checks.)
	bool inDrawBlock;
	
	// Texturing
	bool textureStageDirty[GLW_MAX_TEXTURE_STAGES];
	bool textureStageEnable[GLW_MAX_TEXTURE_STAGES];
	GLuint currentTexture[GLW_MAX_TEXTURE_STAGES];
	D3DTEXTUREOP textureEnv[GLW_MAX_TEXTURE_STAGES];
	
	struct TextureInfo
	{
		IDirect3DTexture8* mipmap;
		D3DTEXTUREFILTERTYPE minFilter, mipFilter, magFilter;
		D3DTEXTUREADDRESS wrapU, wrapV;
		float anisotropy;
	};

	typedef std::map<GLuint, TextureInfo> texturexlat_t;
	texturexlat_t textureXlat;

	GLuint textureBindNum;

	GLuint serverTU, clientTU;

	// Pointers to various draw buffers
	const void* vertexPointer;
	const void* normalPointer;
	const void* texCoordPointer[GLW_MAX_TEXTURE_STAGES];
	const void* colorPointer;

#ifdef _WINDOWS
	// Temporary storage used when rendering quads
	const void* vertexPointerBack;
	const void* normalPointerBack;
	const void* texCoordPointerBack[GLW_MAX_TEXTURE_STAGES];
	const void* colorPointerBack;
#endif

	// State of draw buffers
	bool colorArrayState;
	bool texCoordArrayState[GLW_MAX_TEXTURE_STAGES];
	bool vertexArrayState;
	bool normalArrayState;
	
	// Stride of various draw buffers
	int vertexStride;
	int texCoordStride[GLW_MAX_TEXTURE_STAGES];
	int colorStride;
	int normalStride;

	// Current number of verts in this packet
	int numVertices;

	// Max verts allowed in this packet
	int maxVertices;

	// Total verts to draw (may take multiple packets)
	int totalVertices;

	// Current number of indices in this packet
	int numIndices;

	// Max indices allowed in this packet
	int maxIndices;

	// Total indices to draw
	int totalIndices;

	// Culling
	bool cullEnable;
	D3DCULL cullMode;

	// Viewport
	D3DVIEWPORT8 viewport;

	// Clearing info
	D3DCOLOR clearColor;
	float clearDepth;
	int clearStencil;

	// Widescreen mode
	bool isWidescreen;

	// Global color
	D3DCOLOR currentColor;

	// Scissoring
	bool scissorEnable;
	D3DRECT scissorBox;

	// Directional Light
	D3DLIGHT8	dirLight;
	D3DMATERIAL8 mtrl;

	// Description of current shader
	DWORD shaderMask;

	// Should we reset matrices on next draw?
	bool matricesDirty[Num_MatrixModes];

	// Render commands go here
	DWORD* drawArray;
	DWORD drawStride;

	// This is designed to be an optimization for triangle strips
	// as well as making life easier for the flare effect
	GLushort strip_dest[SHADER_MAX_INDEXES];
	GLuint strip_lengths[GLW_MAX_STRIPS];
	GLsizei num_strip_lengths;

#ifdef _XBOX
//	class FlareEffect*	flareEffect;
	class LightEffects* lightEffects;
#endif
};

extern glwstate_t *glw_state;

void renderObject_HACK();
void renderObject_Light();
void renderObject_Env();
void renderObject_Bump();
bool CreateVertexShader( const CHAR* strFilename, const DWORD* pdwVertexDecl, DWORD* pdwVertexShader );
bool CreatePixelShader( const CHAR* strFilename, DWORD* pdwPixelShader );

#endif
