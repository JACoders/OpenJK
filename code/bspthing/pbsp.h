/*
==============================================================================

  Packed .BSP structures

==============================================================================
*/

#pragma pack(push, 1)
typedef struct {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} pdfog_t;

typedef struct {
	int				firstSide;
	byte			numSides;
	unsigned short	shaderNum;		// the shader that determines the contents flags
} pdbrush_t;

typedef struct {
	int				planeNum;		// positive plane side faces out of the leaf
	byte			shaderNum;
} pdbrushside_t;

typedef struct {
	int				planeNum;
	short			children[2];	// negative numbers are -(leafs+1), not nodes
	short			mins[3];		// for frustom culling
	short			maxs[3];
} pdnode_t;

typedef struct {
	short			cluster;			// -1 = opaque cluster (do I still store these?)
	signed char		area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstLeafSurface;
	unsigned short	numLeafSurfaces;

	unsigned short	firstLeafBrush;
	unsigned short	numLeafBrushes;
} pdleaf_t;

typedef struct {
	float			mins[3], maxs[3];
	int				firstSurface;
	unsigned short	numSurfaces;
	int				firstBrush;
	unsigned short	numBrushes;
} pdmodel_t;

typedef struct {
	byte	flags;
	byte	latLong[2];
	int		data;
} pdgrid_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			surfaceFlags;
	int			contentFlags;
} pdshader_t;

typedef struct {
	float		normal[3];
	float		dist;
} pdplane_t;

typedef struct {
	float			lightmap[MAXLIGHTMAPS][2];
	float			st[2];
	short			xyz[3];
	short			normal[3];
	byte			color[MAXLIGHTMAPS][4];
} pmapVert_t;

typedef struct {
	int				code;
	byte			shaderNum;
	signed char		fogNum;

	unsigned int	verts;				// high 20 bits are first vert, low 12 are num verts

	byte			lightmapStyles[MAXLIGHTMAPS];
	byte			lightmapNum[MAXLIGHTMAPS];

	short			lightmapVecs[2][3];	// for patches, [0] and [1] are lodbounds

	byte			patchWidth;
	byte			patchHeight;
} pdpatch_t;

typedef struct {
	int				code;
	byte			shaderNum;
	signed char		fogNum;

	unsigned int	verts;				// high 20 bits are first vert, low 12 are num verts
	unsigned int	indexes;			// high 20 bits are first index, low 12 are num indices

	byte			lightmapStyles[MAXLIGHTMAPS];
	byte			lightmapNum[MAXLIGHTMAPS];

	short			lightmapVecs[3];
} pdface_t;

typedef struct {
	int				code;
	byte			shaderNum;
	signed char		fogNum;

	unsigned int	verts;				// high 20 bits are first vert, low 12 are num verts
	unsigned int	indexes;			// high 20 bits are first index, low 12 are num indices

	byte			lightmapStyles[MAXLIGHTMAPS];
} pdtrisurf_t;

typedef struct {
	int				code;
	byte			shaderNum;
	signed char		fogNum;

	short			origin[3];
	short			normal[3];
	byte			color[3];
} pdflare_t;

#pragma pack(pop)
