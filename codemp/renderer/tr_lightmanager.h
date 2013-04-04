/*
**	tr_lightmanager.h
*/

#ifndef TR_LIGHTMANAGER_H
#define TR_LIGHTMANAGER_H

#include "tr_local.h"


class VVLightManager {
public:

	VVLightManager();
	void R_TransformDlights( int count, VVdlight_t *dl, orientationr_t *ori);
	void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
	void RE_AddLightToScene( VVdlight_t *light );
	void R_DlightBmodel( bmodel_t *bmodel, qboolean NoLight );
	int  R_DlightFace( srfSurfaceFace_t *face, int dlightBits );
	int  R_DlightGrid( srfGridMesh_t *grid, int dlightBits );
	int  R_DlightTrisurf( srfTriangles_t *surf, int dlightBits );
	int  R_DlightSurface( msurface_t *surf, int dlightBits );
	void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
	void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits );
	void RB_CalcDiffuseColor( DWORD *colors );
	void RB_CalcDiffuseEntityColor( DWORD *colors );
	void ShortToVec3(const short in[3], vec3_t &out);
	int  BoxOnPlaneSide (const short emins[3], const short emaxs[3], struct cplane_s *p);
};

extern VVLightManager VVLightMan; 


#endif