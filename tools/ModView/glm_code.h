// Filename:-	glm_code.h
//


#ifndef GLM_CODE_H
#define GLM_CODE_H

#include "mdx_format.h"



////////////////////////////////////////////////
//
// Jake's stuff...
#define MAX_G2_SURFACES 1000	// this is the max surfaces any model can have. IMPORTANT - used everywhere
#define	MAX_G2_LODS		10		// this restriction mainly a MODVIEW thing, but carcass will only build 10 LODs as well because of surfacename rules (ie "xxxx_n" names)

#define MAX_BONE_OVERRIDES	32
#define MAX_POSSIBLE_BONES	512

#define BONE_ANGLES_ABSOLUTE	0x0001
#define BONE_ANGLES_ADDITIVE	0x0002
#define BONE_ANGLES_RELATIVE	0x0004
#define BONE_ANIM_OVERRIDE		0x0008
#define BONE_ANIM_OVERRIDE_LOOP	0x0010
#define BONE_ANIM_NEW_ANIM		0x0020


typedef struct {
	int		boneNumber;
	vec3_t	angles;
	int		flags;
	int		startFrame;
	int		endFrame;
	float	currentFrame;
	float	newFrame;
	float	animSpeed;
} boneInfo_t;


typedef enum
{
	SURF_ERROR,	// needed to replace where Jake just did "return 0", which couldn't be distinguished from SURF_ON
	SURF_ON,// = SURF_ERROR,
	SURF_OFF,
	SURF_NO_DESCENDANTS,
	SURF_INHERENTLYOFF		// can never be set, but may be returned on query
} SurfaceOnOff_t;								// enumerations for offFlags used in surfaceInfo_t

typedef struct {
	int			ident;			// ident of this surface - required so the materials renderer knows what sort of surface this refers to 
	SurfaceOnOff_t offFlags;		// what the flags are for this model
	int			surface;		// index into array held inside the model definition of pointers to the actual surface data loaded in - used by both client and game
	void	 	*boneList;		// pointer to transformed bone list for this surface - required client side for rendering DONOT USE IN GAME	SIDE
	void		*surfaceData;	// pointer to surface data loaded into file - only used by client renderer DO NOT USE IN GAME SIDE - if there is a vid restart this will be out of wack on the game
} surfaceInfo_t;

/*
typedef struct {
	float		matrix[3][4];
} mdxBone_t;
*/

typedef struct
{
	float		m[4][4];
}mdxBone4_t;


//
////////////////////////////////////////////////


bool			GLMModel_Parse(struct ModelContainer *pContainer, LPCSTR psLocalFilename, HTREEITEM hTreeItem_Parent = NULL);
SurfaceOnOff_t	GLMModel_Surface_GetStatus( ModelHandle_t hModel, int iSurfaceIndex );
void			GLMModel_DeleteExtra(void);
bool			GLMModel_SurfaceContainsBoneReference(ModelHandle_t hModel, int iLODNumber, int iSurfaceNumber, int iBoneNumber);
LPCSTR			GLMModel_BoneInfo( ModelHandle_t hModel, int iBoneIndex );
LPCSTR			GLMModel_SurfaceVertInfo( ModelHandle_t hModel, int iSurfaceIndex );
LPCSTR			GLMModel_SurfaceInfo( ModelHandle_t hModel, int iSurfaceIndex, bool bShortVersionForTag );
bool			GLMModel_SurfaceIsTag(ModelHandle_t hModel, int iSurfaceindex );
bool			GLMModel_SurfaceIsON(ModelHandle_t hModel, int iSurfaceIndex );
LPCSTR			GLMModel_GetSurfaceName( ModelHandle_t hModel, int iSurfaceIndex );
LPCSTR			GLMModel_GetSurfaceShaderName( ModelHandle_t hModel, int iSurfaceIndex );
LPCSTR			GLMModel_GetBoneName( ModelHandle_t hModel, int iBoneIndex );
bool		  R_GLMModel_Tree_ReEvalSurfaceText(ModelHandle_t hModel, HTREEITEM hTreeItem = NULL, bool bDeadFromHereOn = false);
bool			GLMModel_Surface_Off( ModelHandle_t hModel, int iSurfaceIndex );
bool			GLMModel_Surface_On( ModelHandle_t hModel, int iSurfaceIndex );
bool			GLMModel_Surface_NoDescendants(ModelHandle_t hModel, int iSurfaceIndex );
bool			GLMModel_Surface_SetStatus( ModelHandle_t hModel, int iSurfaceIndex, SurfaceOnOff_t eStatus );
void			GLMModel_Surfaces_DefaultAll(ModelHandle_t hModel);
mdxaBone_t	   *GLMModel_GetBasePoseMatrix(ModelHandle_t hModel, int iBoneIndex);
bool			GLMModel_GetBounds(ModelHandle_t hModel, int iLODNumber, int iFrameNumber, vec3_t &v3Mins, vec3_t &v3Maxs);
int 			GLMModel_EnsureGenerated_VertEdgeInfo(ModelHandle_t hModel, int iLOD, SurfaceEdgeInfoPerLOD_t &SurfaceEdgeInfoPerLOD);
void		   *GLMModel_GetDefaultGLA(void);

#endif	// #ifndef GLM_CODE_H


////////////// eof ////////////

