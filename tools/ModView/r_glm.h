// Filename:-	R_GLM.h
//
// basic header to access a pile of code pasted from other projects to get support for this format up and running...
//

#ifndef R_GLM_H
#define	R_GLM_H

extern qboolean R_LoadMDXM (model_t *mod, void *buffer, const char *name );
extern qboolean R_LoadMDXA (model_t *mod, void *buffer, const char *name );

void R_AddGhoulSurfaces( trRefEntity_t *ent );


void RB_SurfaceGhoul( surfaceInfo_t *surf );
qboolean R_LoadMDXM( model_t *mod, void *buffer, const char *mod_name );
qboolean R_LoadMDXA( model_t *mod, void *buffer, const char *mod_name );
void R_GetMDXATag( mdxmHeader_t *mod, int framenum, const char *tagName ,md3Tag_t * dest);
qboolean G2_SetSurfaceOnOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName, const SurfaceOnOff_t offFlags, const int surface);
void G2_GetSurfaceList (qhandle_t model, surfaceInfo_t *slist);
SurfaceOnOff_t G2_IsSurfaceOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName);
int G2_Find_Bone(const model_t *mod, boneInfo_t *blist, const char *boneName);
int G2_Add_Bone (const model_t *mod, boneInfo_t *blist, const char *boneName);
qboolean G2_Remove_Bone_Index ( boneInfo_t *blist, int index);
int	G2_Find_Bone_In_List(boneInfo_t *blist, const int boneNum);
qboolean G2_Stop_Bone_Anim_Index( boneInfo_t *blist, int index);
qboolean G2_Remove_Bone (const qhandle_t model, boneInfo_t *blist, const char *boneName);
qboolean G2_Set_Bone_Angles(const qhandle_t model, boneInfo_t *blist, const char *boneName, const float *angles, const int flags);
qboolean G2_Get_Bone_Angles(const qhandle_t model, boneInfo_t *blist, const char *boneName, float *angles);
qboolean G2_Set_Bone_Anim(const qhandle_t model, boneInfo_t *blist, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed);
qboolean G2_Get_Bone_Anim(const qhandle_t model, boneInfo_t *blist, const char *boneName, float *currentFrame, int *startFrame, int *endFrame);
qboolean G2_Stop_Bone_Anim(const qhandle_t model, boneInfo_t *blist, const char *boneName);
qboolean G2_Stop_Bone_Angles(const qhandle_t model, boneInfo_t *blist, const char *boneName);
void G2_Animate_Bone_List(boneInfo_t *blist, float timeoffset);
void G2_Init_Bone_List(boneInfo_t *blist);



#endif	// #ifndef R_GLM_H

/////////////////////// eof /////////////////////

