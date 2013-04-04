//
//
// win_stencilshadow.h
//
// Declaration for stencil shadowing class
//
//

#ifndef _WIN_STENCILSHADOW_H_
#define _WIN_STENCILSHADOW_H_

typedef struct 
{
	// facing is only one bit, but we can't do better than 4 bytes without
	// packing all the data we need into a single unsigned short, which
	// isn't really worth it. (unless we REALLY need 64k at some point).
	short	i2;
	short	facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	16


class StencilShadow
{
public:

	edgeDef_t	m_edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
	int			m_numEdgeDefs[SHADER_MAX_VERTEXES];
	int			m_facing[SHADER_MAX_INDEXES/3];
	vec3_t		m_shadowVerts[SHADER_MAX_VERTEXES * 8];

	StencilShadow();
	~StencilShadow();
	void AddEdge( int i1, int i2, int facing );
	void RenderEdges();
	bool BuildFromLight( VVdlight_t *dl );
	void RenderShadow();
	void FinishShadows();
};

extern StencilShadow StencilShadower;


#endif