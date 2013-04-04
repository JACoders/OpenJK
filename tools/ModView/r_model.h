// Filename:-	R_Model.h
//
// header file for gateway to block-paste format code that doesn't know it's inside ModView
//
// Note that only functions called by ModView should be in here!!!!!
//	Protos for functions in R_MODEL.CPP should be in R_COMMON.H to hide them from the clean APP code

#ifndef R_MODEL_H
#define R_MODEL_H



void OnceOnlyCrap(void);

void*			RE_GetModelData( ModelHandle_t hModel );
modtype_t		RE_GetModelType( ModelHandle_t hModel );
ModelHandle_t	RE_RegisterModel( const char *name );
void			RE_DeleteModels( void );
void			RE_ModelBinCache_DeleteAll(void);


void			trap_G2_SurfaceOffList	(int a, void *b);
qboolean		trap_G2_SetSurfaceOnOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName, const SurfaceOnOff_t offFlags, const int surface);
SurfaceOnOff_t	trap_G2_IsSurfaceOff	(qhandle_t model, surfaceInfo_t *slist, const char *surfaceName);
void			trap_G2_Init_Bone_List	(void *a);
qboolean		trap_G2_Set_Bone_Anim	(int a, void *b, void *c, int d, int e, int f, float g);


#endif	// #ifndef R_MODEL_H


//////////////////// eof /////////////////



