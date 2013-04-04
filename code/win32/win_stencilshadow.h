//
//
// win_stencilshadow.h
//
// Declaration for stencil shadowing class
//
//

#ifndef _WIN_STENCILSHADOW_H_
#define _WIN_STENCILSHADOW_H_

// Quick way turn off all stencilshadow code, and get back 170k of memory:
//#define DISABLE_STENCILSHADOW

// Enable this to enable Carmack's stencil reverse method
#define _STENCIL_REVERSE

typedef struct 
{
	// facing is only one bit, but we can't do better than 4 bytes without
	// packing all the data we need into a single unsigned short, which
	// isn't really worth it. (unless we REALLY need 64k at some point).
	unsigned short	i2;
	byte	facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	16


class StencilShadow
{
public:

#ifndef DISABLE_STENCILSHADOW
	edgeDef_t		m_edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
	short			m_numEdgeDefs[SHADER_MAX_VERTEXES];
	byte			m_facing[SHADER_MAX_INDEXES/3];
	unsigned short  m_shadowIndexes[SHADER_MAX_INDEXES];
	unsigned short	m_nIndexes;
	float			m_extrusionIndicators[SHADER_MAX_VERTEXES/2];
#ifdef _STENCIL_REVERSE
	unsigned short  m_shadowIndexesCap[SHADER_MAX_INDEXES];
	unsigned short  m_nIndexesCap;
#endif

#endif
	DWORD		m_dwVertexShaderShadow;
	DWORD		*pVerts;	// For reusing vertices in multiple shadow renderings
	DWORD		*pExtrusions;

	StencilShadow();
	~StencilShadow();
	bool Initialize();
	void AddEdge( unsigned short i1, unsigned short i2, byte facing );
	void BuildEdges();
	bool BuildFromLight();
	void RenderShadow();
	void FinishShadows();
};

extern StencilShadow StencilShadower;

#endif