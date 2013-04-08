/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#include "../game/q_shared.h"
#include "qcommon.h"
#include "cm_polylib.h"
#include "cm_landscape.h"

#ifdef _XBOX
#include "sparc.h"
#endif

#ifndef CM_LOCAL_H
#define CM_LOCAL_H

#define	BOX_MODEL_HANDLE	(MAX_SUBMODELS-1)

#ifdef _XBOX
#pragma pack(push, 1)
typedef struct {
	short		children[2];		// negative numbers are leafs
} cNode_t;
#pragma pack(pop)

#else // _XBOX

typedef struct {
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
} cNode_t;

#endif // _XBOX

typedef struct {
	int			cluster;
	int			area;

	int			firstLeafBrush;
	int			numLeafBrushes;

	int			firstLeafSurface;
	int			numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s {
	vec3_t		mins, maxs;
	cLeaf_t		leaf;			// submodels don't reference the main tree
} cmodel_t;

#ifdef _XBOX
#pragma pack (push, 1)
typedef struct cbrushside_s {
	NotSoShort planeNum;
	unsigned char	shaderNum;
} cbrushside_t;
#pragma pack(pop)

#else // _XBOX

typedef struct cbrushside_s {
	cplane_t	*plane;
	int			shaderNum;
} cbrushside_t;

#endif // _XBOX

typedef struct cbrush_s {
	int					shaderNum;		// the shader that determined the contents
	int					contents;
	vec3_t				bounds[2];
	cbrushside_t		*sides;
	unsigned short		numsides;
	unsigned short		checkcount;		// to avoid repeated testings
} cbrush_t;

class CCMShader
{
public:
	char				shader[MAX_QPATH];
	class CCMShader		*mNext;
	int					surfaceFlags;
	int					contentFlags;

	const char			*GetName(void) const { return(shader); }
	class CCMShader		*GetNext(void) const { return(mNext); }
	void				SetNext(class CCMShader *next) { mNext = next; }
	void				Destroy(void) { }
};

typedef struct {
	int			checkcount;				// to avoid repeated testings
	int			surfaceFlags;
	int			contents;
	struct patchCollide_s	*pc;
} cPatch_t;


typedef struct {
	int			floodnum;
	int			floodvalid;
} cArea_t;

#ifdef _XBOX
template <class T>
class SPARC;
typedef struct {
	char		name[MAX_QPATH];

	int			numShaders;
	CCMShader	*shaders;

	int			numBrushSides;
	cbrushside_t *brushsides;

	int			numPlanes;
	cplane_t	*planes;

	int			numNodes;
	cNode_t		*nodes;

	int			numLeafs;
	cLeaf_t		*leafs;

	int			numLeafBrushes;
	int			*leafbrushes;

	int			numLeafSurfaces;
	int			*leafsurfaces;

	int			numSubModels;
	cmodel_t	*cmodels;

	int			numBrushes;
	cbrush_t	*brushes;

	int			numClusters;
	int			clusterBytes;
	SPARC<byte>	*visibility;
	qboolean	vised;			// if false, visibility is just a single cluster of ffs

	int			numEntityChars;
	char		*entityString;

	int			numAreas;
	cArea_t		*areas;
	int			*areaPortals;	// [ numAreas*numAreas ] reference counts

	int			numSurfaces;
	cPatch_t	**surfaces;			// non-patches will be NULL

	int			floodvalid;
	int			checkcount;					// incremented on each trace

//	CCMLandScape	*landScape;		// Removing terrain from Xbox
} clipMap_t;

#else // _XBOX

typedef struct {
	char		name[MAX_QPATH];

	int			numShaders;
	//dshader_t	*shaders;
	CCMShader	*shaders;

	int			numBrushSides;
	cbrushside_t *brushsides;

	int			numPlanes;
	cplane_t	*planes;

	int			numNodes;
	cNode_t		*nodes;

	int			numLeafs;
	cLeaf_t		*leafs;

	int			numLeafBrushes;
	int			*leafbrushes;

	int			numLeafSurfaces;
	int			*leafsurfaces;

	int			numSubModels;
	cmodel_t	*cmodels;

	int			numBrushes;
	cbrush_t	*brushes;

	int			numClusters;
	int			clusterBytes;
	byte		*visibility;
	qboolean	vised;			// if false, visibility is just a single cluster of ffs

	int			numEntityChars;
	char		*entityString;

	int			numAreas;
	cArea_t		*areas;
	int			*areaPortals;	// [ numAreas*numAreas ] reference counts

	int			numSurfaces;
	cPatch_t	**surfaces;			// non-patches will be NULL

	int			floodvalid;
	int			checkcount;					// incremented on each trace

	CCMLandScape	*landScape;
} clipMap_t;

#endif // _XBOX


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define	SURFACE_CLIP_EPSILON	(0.125)

extern	clipMap_t	cmg;
extern	int			c_pointcontents;
extern	int			c_traces, c_brush_traces, c_patch_traces;
extern	cvar_t		*cm_noAreas;
extern	cvar_t		*cm_noCurves;
extern	cvar_t		*cm_playerCurveClip;

extern clipMap_t	SubBSP[MAX_SUB_BSP];
extern int			NumSubBSP;

// cm_test.c

// Used for oriented capsule collision detection
typedef struct
{
	qboolean	use;
	float		radius;
	float		halfheight;
	vec3_t		offset;
} sphere_t;

typedef struct traceWork_s {
	vec3_t		start;
	vec3_t		end;
	vec3_t		size[2];	// size of the box being swept through the model
	vec3_t		offsets[8];	// [signbits][x] = either size[0][x] or size[1][x]
	float		maxOffset;	// longest corner length from origin
	vec3_t		extents;	// greatest of abs(size[0]) and abs(size[1])

	vec3_t		bounds[2];	// enclosing box of start and end surrounding by size
	vec3pair_t	localBounds;	// enclosing box of start and end surrounding by size for a segment

	vec3_t		modelOrigin;// origin of the model tracing through
	int			contents;	// ored contents of the model tracing through
	qboolean	isPoint;	// optimized case
	sphere_t	sphere;		// sphere for oriendted capsule collision

	float			baseEnterFrac;	// global enter fraction (before processing subsections of the brush)	
	float			baseLeaveFrac;	// global leave fraction (before processing subsections of the brush)	
	float			enterFrac;		// fraction where the ray enters the brush
	float			leaveFrac;		// fraction where the ray leaves the brush
	cbrushside_t	*leadside;
	cplane_t		*clipplane;
	bool			startout;
	bool			getout;

	trace_t		trace;		// returned from trace call
	// make sure nothing goes under here for Ghoul2 collision purposes
} traceWork_t;

typedef struct leafList_s {
	int		count;
	int		maxcount;
	qboolean	overflowed;
	int		*list;
	vec3_t	bounds[2];
	int		lastLeaf;		// for overflows where each leaf can't be stored individually
	void	(*storeLeafs)( struct leafList_s *ll, int nodenum );
} leafList_t;


int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t **boxlist, int listsize );

void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle, clipMap_t **clipMap = 0 );
void CM_CleanLeafCache(void);

// cm_load.c
void CM_ModelBounds( clipMap_t &cm, clipHandle_t model, vec3_t mins, vec3_t maxs );

// cm_patch.c

struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_ClearLevelPatches( void );

// cm_shader.cpp
void CM_SetupShaderProperties( void );
void CM_ShutdownShaderProperties(void);
CCMShader *CM_GetShaderInfo( const char *name );
CCMShader *CM_GetShaderInfo( int shaderNum );
void		CM_GetModelFormalName ( const char* model, const char* skin, char* name, int size );

//cm_trace.cpp
void CM_CalcExtents(const vec3_t start, const vec3_t end, const traceWork_t *tw, vec3pair_t bounds);
void CM_HandlePatchCollision(struct traceWork_s *tw, trace_t &trace, const vec3_t tStart, const vec3_t tEnd, CCMPatch *patch, int checkcount);
bool CM_GenericBoxCollide(const vec3pair_t abounds, const vec3pair_t bbounds);


//RM_Terrain.cpp
int Round(float value);

//random utils for cm_terrain (and others?)
#define VectorInc(v)					((v)[0] += 1.0f,(v)[1] += 1.0f,(v)[2] +=1.0f)
#define VectorDec(v)					((v)[0] -= 1.0f,(v)[1] -= 1.0f,(v)[2] -=1.0f)
#define VectorInverseScaleVector(a,b,c)	((c)[0]=(a)[0]/(b)[0],(c)[1]=(a)[1]/(b)[1],(c)[2]=(a)[2]/(b)[2])
#define VectorScaleVectorAdd(c,a,b,o)	((o)[0]=(c)[0]+((a)[0]*(b)[0]),(o)[1]=(c)[1]+((a)[1]*(b)[1]),(o)[2]=(c)[2]+((a)[2]*(b)[2]))

#define minimum(x,y) ((x)<(y)?(x):(y))
#define maximum(x,y) ((x)>(y)?(x):(y))

#endif
