/*
**	tr_lightmanager.h
*/

#ifndef TR_LIGHTMANAGER_H
#define TR_LIGHTMANAGER_H

#include "tr_local.h"

#define MAX_NUM_STATIC_LIGHTS	256

enum VVlight_type {
	LT_DIRECTIONAL,
	LT_POINT,
	LT_SPOT
};

typedef struct VVdlight_s {
	VVlight_type type;
	vec3_t		 origin;
	vec3_t		 direction;
	vec3_t		 color;	
	vec3_t		 transformed;
	float		 radius;
	float		 attenuation;
} VVdlight_t;

typedef struct VVslight_s {
	vec3_t		origin;
	vec3_t		color;
	float		radius;
} VVslight_t;


class VVLightManager {
public:

	int				num_dlights;
	int				num_slights;
	VVdlight_t		dlights[MAX_DLIGHTS];
	/*VVslight_t		slights[MAX_NUM_STATIC_LIGHTS];
	unsigned char	slightBits[MAX_NUM_STATIC_LIGHTS];*/
	int				currentlight;

	VVLightManager();
	void R_TransformDlights( orientationr_t *orient);
	void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
	void RE_AddLightToScene( VVdlight_t *light );
	//void RE_AddStaticLightToScene( VVslight_t *light );
	void R_DlightBmodel( bmodel_t *bmodel, qboolean NoLight );
	int  R_DlightFace( srfSurfaceFace_t *face, int dlightBits );
	//void R_SlightFace( srfSurfaceFace_t *face );
	int  R_DlightGrid( srfGridMesh_t *grid, int dlightBits );
	//void R_SlightGrid( srfGridMesh_t *grid );
	int  R_DlightTrisurf( srfTriangles_t *surf, int dlightBits );
	//void R_SlightTrisurf( srfTriangles_t *surf );
	int  R_DlightSurface( msurface_t *surf, int dlightBits );
	void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
	void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits );
	void RB_CalcDiffuseColorWorld();
	void RB_CalcDiffuseColor( DWORD *colors );
	void RB_CalcDiffuseEntityColor( DWORD *colors );
	void ShortToVec3(const short in[3], vec3_t &out);
	int  BoxOnPlaneSide (const short emins[3], const short emaxs[3], struct cplane_s *p);
};

extern VVLightManager VVLightMan; 


#endif