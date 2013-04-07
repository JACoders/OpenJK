#ifndef __CM_PUBLIC_H__
#define __CM_PUBLIC_H__

#include "qfiles.h"

qboolean CM_DeleteCachedMap(qboolean bGuaranteedOkToDelete);
#ifdef _XBOX
void		CM_LoadMap( const char *name, qboolean clientload, int *checksum);
#else
void		CM_LoadMap( const char *name, qboolean clientload, int *checksum, qboolean subBSP);
#endif
void		CM_ClearMap( void );
int			CM_TotalMapContents();

clipHandle_t CM_InlineModel( int index );		// 0 = world, 1 + are bmodels
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );//, const int contents );

int		CM_ModelContents( clipHandle_t model, int subBSPIndex );


int			CM_NumClusters (void);
int			CM_NumInlineModels( void );
char		*CM_EntityString (void);
char		*CM_SubBSPEntityString (int index);
int			CM_LoadSubBSP(const char *name, qboolean clientload);
int			CM_FindSubBSP(int modelIndex);

// returns an ORed contents mask
int			CM_PointContents( const vec3_t p, clipHandle_t model );
int			CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );

void		CM_BoxTrace ( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask);
void		CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles);

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

int			CM_LeafCluster (int leafnum);
int			CM_LeafArea (int leafnum);

void		CM_AdjustAreaPortalState( int area1, int area2, qboolean open );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );

//for savegames
void		CM_WritePortalState ();
void		CM_ReadPortalState ();

// cm_marks.c
int	CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

// cm_patch.c
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, float *points) );

#endif //__CM_PUBLIC_H__
