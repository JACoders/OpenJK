#include "../game/q_shared.h"
#include "qfiles.h"

#ifdef _XBOX
void		CM_LoadMap( const char *name, qboolean clientload, int *checksum);
#else
void		CM_LoadMap( const char *name, qboolean clientload, int *checksum);
#endif

void		CM_ClearMap( void );
clipHandle_t CM_InlineModel( int index );		// 0 = world, 1 + are bmodels
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule );

void		CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );

int			CM_NumClusters (void);
int			CM_NumInlineModels( void );
char		*CM_EntityString (void);

// returns an ORed contents mask
int			CM_PointContents( const vec3_t p, clipHandle_t model );
int			CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );

void		CM_BoxTrace ( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask, int capsule );
void		CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles, int capsule );

#ifdef _XBOX
const byte	*CM_ClusterPVS (int cluster);
#else
byte		*CM_ClusterPVS (int cluster);
#endif

int			CM_PointLeafnum( const vec3_t p );

// only returns non-solid leafs
// overflow if return listsize and if *lastLeaf != list[listsize-1]
int			CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *boxList,
		 					int listsize, int *lastLeaf );
//rwwRMG - changed to boxList to not conflict with list type

int			CM_LeafCluster (int leafnum);
int			CM_LeafArea (int leafnum);

void		CM_AdjustAreaPortalState( int area1, int area2, qboolean open );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );

//rwwRMG - added:
bool		CM_GenericBoxCollide(const vec3pair_t abounds, const vec3pair_t bbounds);
void		CM_HandlePatchCollision(struct traceWork_s *tw, trace_t &trace, const vec3_t tStart, const vec3_t tEnd, class CCMPatch *patch, int checkcount);
void		CM_CalcExtents(const vec3_t start, const vec3_t end, const struct traceWork_s *tw, vec3pair_t bounds);

// cm_tag.c
int			CM_LerpTag( orientation_t *tag,  clipHandle_t model, int startFrame, int endFrame, 
					 float frac, const char *tagName );


// cm_marks.c
int	CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

// cm_patch.c
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, float *points) );

// cm_shader.cpp
const char *CM_GetShaderText(const char *key);
void CM_FreeShaderText(void);
void CM_LoadShaderText(qboolean forceReload);
