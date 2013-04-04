// Filename:-	model.h
//


#ifndef MODEL_H
#define MODEL_H

#include "stl.h"


////////////////////////////////////////////////
//
typedef struct
{
	union
	{
		struct
		{
			unsigned int iItemType		: 8;	// allows 256 item types (see #defines below)
			unsigned int iModelHandle	: 8;	// allows 256 models
			unsigned int iItemNumber	: 16;	// allows 65536 surfaces, bones, sequences etc
		};
		//
		UINT32 uiData;
	};
} TreeItemData_t;


// max 256 of these...
//
typedef enum
{
	TREEITEMTYPE_NULL=0,			// nothing, ie usually a reasonable default for clicking on emptry tree space
	TREEITEMTYPE_MODELNAME,			// "modelname"
	TREEITEMTYPE_SURFACEHEADER,		// "surfaces"
	TREEITEMTYPE_TAGSURFACEHEADER,	// "tag surfaces"
	TREEITEMTYPE_BONEHEADER,		// "bones"
	TREEITEMTYPE_BONEALIASHEADER,	// "bone aliases"
	TREEITEMTYPE_SEQUENCEHEADER,	// "sequences"
	TREEITEMTYPE_BOLTONSHEADER,		// "BoltOns"
	TREEITEMTYPE_SKINSHEADER,		// "Skins available"	
	TREEITEMTYPE_OLDSKINSHEADER,	// "Skins available"
	//
	// Ones beyond here should have updated code in ModelTree_GetItemText() to handle pure enquiries if nec.
	//
	TREEITEMTYPE_GLM_SURFACE,		// a surface	(index in bottom bits, currently allows 65535 surfaces)
	TREEITEMTYPE_GLM_TAGSURFACE,	// a surface	(index in bottom bits, currently allows 65535 surfaces)
	TREEITEMTYPE_GLM_BONE,			// a bone		(index in bottom bits, currently allows 65535 bones)
	TREEITEMTYPE_GLM_BONEALIAS,		// a bone alias	(index in bottom bits, currently allows 65535 aliases)	
	TREEITEMTYPE_SEQUENCE,			// a sequence	(index in bottom bits, currently allows 65535 bones)
	TREEITEMTYPE_SKIN,				// (eg "thug")
	TREEITEMTYPE_SKINETHNIC,		// (eg "white")
	TREEITEMTYPE_SKINMATERIAL,		// (eg "face")
	TREEITEMTYPE_SKINMATERIALSHADER,// (eg "models/characters/face/thug1")
	TREEITEMTYPE_OLDSKIN,			// (eg "blue")

} TreeTypes_e;

//
////////////////////////////////////////////////
struct ModelContainer;
typedef struct ModelContainer ModelContainer_t;

typedef vector <bool> VertIsEdge_t;
typedef map <int, VertIsEdge_t> SurfaceEdgeVertBools_t;
typedef map <int, SurfaceEdgeVertBools_t> SurfaceEdgeInfoPerLOD_t;

#include "glm_code.h"

// I wanted to put this in sequence.h, but C is crap for interdependant compiles
//
typedef struct
{
	char	sName[MAX_QPATH];	// eg "run1"
	char	sNameWithPath[2048];	// new, for tree-display purposes only. These can be REALLY long for SOF2 on drive M:
	int		iStartFrame;
	int		iFrameCount;
	int		iLoopFrame;			// -1 for no wrap, else framenum to add to iStartFrame
	int		iFPS;				// can be -ve for CHC to indicate backwards anims, not currently used for speed purposes though
//	bool	bMultiSeq;
	bool	bIsDefault;			// only true if no anim/enum file found
} Sequence_t;

typedef vector <Sequence_t> SequenceList_t;

#include "skins.h"
#include "oldskins.h"


// I need this structure to carry any extra baggage around such as Jake's wacky bone stuff for G2...
//
// update ModelContainer_Clear() when you update this struct!
//
// (Note that these same equates are used for either surfaces or bones, but some only make sense for one or the other)
//
#define iITEMHIGHLIGHT_ALL_TAGSURFACES -4
#define iITEMHIGHLIGHT_ALIASED	-3
#define iITEMHIGHLIGHT_ALL		-2
#define iITEMHIGHLIGHT_NONE		-1
//#define iBONEHIGHLIGHT_all_other_numbers_from_0_to_(n)	// :-)

typedef struct
{
	float fMat[16];
} MyGLMatrix_t;

typedef struct // currently destroyed within ModelContainer_Clear(), allocated (for bolt ons) within ModelContainer_RegisterModel()
{
/*
	//ModelContainer_t	   *pBoltedContainer;
	vector <ModelContainer_t *> vpBoltedContainers;
	string					sAttachName;
	//
//	vector < MyGLMatrix_t >	MatricesPerFrame;	// why the fuck doesn't this work?  Dammit.
	MyGLMatrix_t			*pMatrices;
*/
	string						sAttachName;
	vector <MyGLMatrix_t>		vMatricesPerFrame;	// why the fuck doesn't this work?  Dammit.
	vector <ModelContainer_t>	vBoltedContainers;

} BoltPoint_t;

typedef map <string, string> MappedString_t;
typedef map <string, GLuint> MaterialBinds_t;
struct ModelContainer
{
	ModelHandle_t	hModel;
	modtype_t		eModType;
	char			sLocalPathName[MAX_QPATH];	
	//
	// workspace stuff that G2 models need...
	//
	surfaceInfo_t	slist[MAX_G2_SURFACES];	// this is the surface list for a model - most of this won't be changing so it shouldn't impact networking that badly
	boneInfo_t		blist[MAX_BONE_OVERRIDES];	// this is the list of bones we want to override features on. This *shouldn't* change that much - networking will take a bit of a hit on this
	int				iBoneNum_SecondaryStart;	//-1 by default, meaning "ignore", else bone num to stop animating at from upper (eg "lower_lumbar"), and start animating from lower downwards
	int				iSurfaceNum_RootOverride;	//-1 by default, meaning "ignore", else surf num to start rendering from
	//
	// a couple of tacky things that're used for bone-view highlighting and bolting
	//
	mdxaBone_t		XFormedG2Bones			[MAX_POSSIBLE_BONES];
	bool			XFormedG2BonesValid		[MAX_POSSIBLE_BONES];
	mdxaBone_t		XFormedG2TagSurfs		[MAX_G2_SURFACES];	// same thing for surface-bolting
	bool			XFormedG2TagSurfsValid	[MAX_G2_SURFACES];

	// linkage/bolting fields...
	//	
	// bone bolts...
	//
	ModelContainer_t *pBoneBolt_ParentContainer;	// NULL if root model, else parent we're bolted to
	int				iBoneBolt_ParentBoltIndex;			// index of which bolt point on parent we're bolted to
	int				iBoneBolt_MaxBoltPoints;					// currently will always be MAX_BOLTS, but may change on a per-model basis
	vector <BoltPoint_t> tBoneBolt_BoltPoints;
	//
	// surface bolts...
	//
	ModelContainer_t *pSurfaceBolt_ParentContainer;	// NULL if root model, else parent we're bolted to
	int				iSurfaceBolt_ParentBoltIndex;			// index of which bolt point on parent we're bolted to
	int				iSurfaceBolt_MaxBoltPoints;					// currently will always be MAX_BOLTS, but may change on a per-model basis
	vector <BoltPoint_t> tSurfaceBolt_BoltPoints;
	
	// stuff for viewer info / control etc for this model...
	//
	int				iCurrentFrame_Primary;	// for anim playback
	int				iCurrentFrame_Secondary;
	int				iOldFrame_Primary;	
	int				iOldFrame_Secondary;
	int				iSequenceLockNumber_Primary;	// -1 for none, else sequence number that's locked
	int				iSequenceLockNumber_Secondary;	// -1 for none, else sequence number that's locked
	int				iNumFrames;		// for easy frame capping
	int				iNumLODs;
	SequenceList_t	SequenceList;
	bool			bSeqMultiLock_Primary_Active;
	bool			bSeqMultiLock_Secondary_Active;
	int				iSeqMultiLock_Primary_SeqHint;	// slightly odd usage, and self-correcting if illegal
	int				iSeqMultiLock_Secondary_SeqHint;//
	vector <int>	SeqMultiLock_Primary;
	vector <int>	SeqMultiLock_Secondary;
	SkinSets_t		SkinSets;		// SOF2-format skins
	SkinSetsSurfacePrefs_t SkinSetsSurfacePrefs;	// SOF2-format skin surface hints
	OldSkinSets_t	OldSkinSets;	// CHC-format skins

	SurfaceEdgeInfoPerLOD_t SurfaceEdgeInfoPerLOD;	// used only when crack-viewing

	string			strCurrentSkinFile;		// used for both SOF and CHC skins
	string			strCurrentSkinEthnic;	// only for SOF2 skins
	MaterialBinds_t	MaterialBinds;		// constructed whenever skin details are changed, used for fast-lookup
	MappedString_t	MaterialShaders;	// " "	
	//
	// more freaky stuff to do with MODVIEW, rather than the format as a whole...
	//
	HTREEITEM		hTreeItem_ModelName;	// filled in by format-specific parser, so we can add a "BoltOns" treeitem later
	HTREEITEM		hTreeItem_BoltOns;		// attached underneath hTreeItem_ModelName, and passed as parent tree item to bolted containers
	//
	// these 2 are just for viewing bone highlights and shouldn't be used for anything else...
	//
	int				iNumBones;				// zero for all model types other than ghoul2?, only used for loop-speed opt
	int				iBoneHighlightNumber;	// only affects viewing of ghoul2 model types, see equates above
	MappedString_t	Aliases;				// keyed [realname] = aliasname, opposite to what you'd expect	
	//
	// semi-ditto here...
	//
	int				iNumSurfaces;
	int				iSurfaceHighlightNumber;
//	int				iRenderedBoneWeightsThisSurface;
	//
	// stats only...
	//
	int				iRenderedTris;
	int				iRenderedVerts;
	int				iRenderedSurfs;
	int				iXformedG2Bones;
	int				iRenderedBoneWeights;
	int				iOmittedBoneWeights;

	// some function ptrs to be filled in by specific model types...
	//
	LPCSTR			(*pModelInfoFunction)				( ModelHandle_t hModel );
	LPCSTR			(*pModelGetBoneNameFunction)		( ModelHandle_t hModel, int iBoneIndex );
	LPCSTR			(*pModelGetBoneBoltNameFunction)	( ModelHandle_t hModel, int iBoltIndex );		// same as above in GLM model
	LPCSTR			(*pModelGetSurfaceNameFunction)		( ModelHandle_t hModel, int iSurfaceIndex );
	LPCSTR			(*pModelGetSurfaceBoltNameFunction)	( ModelHandle_t hModel, int iSurfaceIndex );// same as above in GLM model
};


const double ANIM_SLOWER = 1.3;
const double ANIM_FASTER = 0.9;

typedef struct
{
	CString			strLoadedModelPath;	// full disk path of primary model
	
//	int					iNumContainers;	 //deleteme
	int					iTotalContainers;
	ModelContainer_t	Container;
	
	//
	// user settings...
	//
	bool	bBilinear;
	bool	bOriginLines;
	bool	bBBox;
	bool	bFloor;
	float	fFloorZ;
	bool	bRuler;
	bool	bAnimate;
	bool	bForceWrapWhenAnimating;
	bool	bInterpolate;
	bool	bUseAlpha;
	bool	bWireFrame;
	bool	bBoneHighlight;
	bool	bBoneWeightThreshholdingActive;
	float	fBoneWeightThreshholdPercent;
	bool	bSurfaceHighlight;
	bool	bSurfaceHighlightShowsBoneWeighting;
	bool	bTriIndexes;
	bool	bVertIndexes;
	bool	bVertWeighting;
	bool	bAtleast1VertWeightDisplayed;	// tacky, but useful for reducing screen clutter
	bool	bVertexNormals;
	bool	bShowTagSurfaces;
	bool	bShowOriginsAsRGB;
	bool	bForceWhite;
	bool	bCleanScreenShots;
	bool	bFullPathsInSequenceTreeitems;
	bool	bCrackHighlight;
	bool	bShowUnshadowableSurfaces;

	int		iLOD;

	byte	_R, _G, _B;	// CLS colour

	double	dFOV;
	double	dAnimSpeed;
	double	dTimeStamp1;
	float	fFramefrac;

	float	xPos, yPos, zPos;
	float	rotAngleX, rotAngleY, rotAngleZ;

	float	xPos_SCROLL, yPos_SCROLL, zPos_SCROLL;
	float	rotAngleX_SCROLL, rotAngleY_SCROLL, rotAngleZ_SCROLL;


	// other crap...
	//
	HWND	hWnd;
	bool	bFinished;	// true only at final app shutdown, may be useful for some checks
	bool	bAllowGLAOverrides;
	bool	bShowPolysAsDoubleSided;
	bool	bAlwaysOnTop;
	bool	bSortSequencesByAlpha;
	
	//  some really tacky stuff...
	//
	int		iSurfaceNumToHighlight;	// only valid when  this->bSurfaceHighlight == true
	ModelHandle_t hModelToHighLight;
	ModelHandle_t hModelLastLoaded;	// useful for some simple batch stuff, may only be temp?
  
} ModViewAppVars_t;

extern ModViewAppVars_t AppVars;

void AppVars_Init(void);
void AppVars_ResetViewParams(void);
void App_OnceOnly(void);

ModelHandle_t	Model_GetPrimaryHandle(void);
bool	Model_Loaded(ModelHandle_t hModel = NULL);
void	Model_Delete(void);
void	Model_ValidateSkin( ModelHandle_t hModel, int iSkinNumber);
void	Model_ApplyOldSkin( ModelHandle_t hModel, LPCSTR psSkin );
void	Model_ApplyEthnicSkin(ModelHandle_t hModel, LPCSTR psSkin, LPCSTR psEthnic, bool bApplySurfacePrefs, bool bDefaultSurfaces );
bool	Model_SkinHasSurfacePrefs( ModelHandle_t hModel, LPCSTR psSkin );
void	Model_ApplySkinShaderVariant( ModelHandle_t hModel, LPCSTR psSkin, LPCSTR psEthnic, LPCSTR psMaterial, int iVariant );
LPCSTR	Model_GetSupportedTypesFilter(bool bScriptsEtcAlsoAllowed = false);
bool	Model_DeleteBoltOn(ModelHandle_t hModel, int iBoltPointToDelete, bool bBoltIsBone, int iBoltOnAtBoltPoint);
bool	Model_DeleteBoltOn(ModelContainer_t *pContainer, int iBoltPointToDelete, bool bBoltIsBone, int iBoltOnAtBoltPoint);
bool	Model_DeleteBoltOn(ModelContainer_t *pContainer);
bool	Model_DeleteBoltOn(ModelHandle_t hModelBoltOn);
bool	Model_HasParent(ModelHandle_t hModel);
int		Model_CountItemsBoltedHere(ModelHandle_t hModel, int iBoltindex, bool bBoltIsBone);
bool	Model_LoadPrimary(LPCSTR psFullPathedFilename);
bool	Model_Save(LPCSTR psFullPathedFilename);
bool	Model_LoadBoltOn(LPCSTR psFullPathedFilename, ModelHandle_t hModel, int iBoltIndex, bool bBoltIsBone, bool bBoltReplacesAllExisting);
bool	Model_LoadBoltOn(LPCSTR psFullPathedFilename, ModelHandle_t hModel, LPCSTR psBoltName, bool bBoltIsBone, bool bBoltReplacesAllExisting);
int		Model_GetNumBoneAliases(ModelHandle_t hModel);
bool	Model_GetBoneAliasPair(ModelHandle_t hModel, int iAliasIndex, string &strRealName,string &strAliasName);
bool	ModelContainer_GetBoneAliasPair(ModelContainer_t *pContainer, int iAliasIndex, string &strRealName,string &strAliasName);
int		Model_GetNumSequences(ModelHandle_t hModel);					// remote helper func only
LPCSTR	Model_GetSequenceString(ModelHandle_t hModel, int iSequenceNum);// remote helper func only
bool	Model_SetSecondaryAnimStart(ModelHandle_t hModel, int iBoneNum);
bool	Model_SetSecondaryAnimStart(ModelHandle_t hModel, LPCSTR psBoneName);
int		Model_GetSecondaryAnimStart(ModelHandle_t hModel);
LPCSTR	Model_Info( ModelHandle_t hModel );
void	Model_StartAnim(bool bForceWrap = false );
void	Model_StopAnim();
float	Model_GetLowestPointOnPrimaryModel(void);
LPCSTR	Model_GetSurfaceName( ModelHandle_t hModel, int iSurfaceIndex );
LPCSTR	Model_GetSurfaceName( ModelContainer_t *pContainer, int iSurfaceIndex );
bool	Model_SurfaceIsTag( ModelContainer_t *pContainer, int iSurfaceIndex);
bool	Model_SurfaceIsTag( ModelHandle_t hModel, int iSurfaceIndex);
LPCSTR	Model_GetBoneName( ModelHandle_t hModel, int iBoneIndex );
int		Model_GetBoltIndex( ModelHandle_t hModel, LPCSTR psBoltName, bool bBoltIsBone);
int		Model_GetBoltIndex( ModelContainer_t *pContainer, LPCSTR psBoltName, bool bBoltIsBone );
LPCSTR	Model_GetBoltName( ModelHandle_t hModel, int iBoltIndex, bool bBoltIsBone );
LPCSTR	Model_GetBoltName( ModelContainer_t *pContainer, int iBoltIndex, bool bBoltIsBone );
LPCSTR	Model_GetFullPrimaryFilename( void );
LPCSTR	Model_GetFilename( ModelHandle_t hModel );
LPCSTR	Model_GLMBoneInfo( ModelHandle_t hModel, int iBoneIndex );
LPCSTR	Model_GLMSurfaceInfo( ModelHandle_t hModel, int iSurfaceIndex, bool bShortVersionForTag );
LPCSTR	Model_GLMSurfaceVertInfo( ModelHandle_t hModel, int iSurfaceIndex );
bool	Model_SurfaceContainsBoneReference(ModelHandle_t hModel, int iLODNumber, int iSurfaceNumber, int iBoneNumber);
bool	Model_GLMSurface_Off(ModelHandle_t hModel, int iSurfaceIndex );
bool	Model_GLMSurface_On(ModelHandle_t hModel, int iSurfaceIndex );
bool	Model_GLMSurface_NoDescendants(ModelHandle_t hModel, int iSurfaceIndex );
bool	Model_GLMSurface_SetStatus( ModelHandle_t hModel, int iSurfaceIndex, SurfaceOnOff_t eStatus );
bool	Model_GLMSurface_SetStatus( ModelHandle_t hModel, LPCSTR psSurfaceName, SurfaceOnOff_t eStatus );
void	Model_GLMSurfaces_DefaultAll(ModelHandle_t hModel);
SurfaceOnOff_t Model_GLMSurface_GetStatus( ModelHandle_t hModel, int iSurfaceIndex );
int		Model_GetNumSurfacesDifferentFromDefault(ModelContainer_t *pContainer, SurfaceOnOff_t eStatus);
LPCSTR	Model_GetSurfaceDifferentFromDefault(ModelContainer_t *pContainer, SurfaceOnOff_t eStatus, int iSurfaceIndex);
bool	Model_SetBoneHighlight(ModelHandle_t hModel, int iBoneIndex);
bool	Model_SetBoneHighlight(ModelHandle_t hModel, LPCSTR psBoneName);
bool	Model_SetSurfaceHighlight(ModelHandle_t hModel, int iSurfaceindex);
bool	Model_SetSurfaceHighlight(ModelHandle_t hModel, LPCSTR psSurfaceName);
int		Model_EnsureGenerated_VertEdgeInfo(ModelContainer_t *pContainer, int iLOD);
int		Model_EnsureGenerated_VertEdgeInfo(ModelHandle_t hModel, int iLOD);

void	ModelList_ForceRedraw(void);
void	ModelList_Render(int iWindowWidth, int iWindowHeight);
void	ModelList_Rewind(void);
void	ModelList_GoToEndFrame();
bool	ModelList_StepFrame(int iStepVal, bool bAutoAnimOff = true);
bool	ModelList_Animation(void);

void		ModelTree_DeleteAllItems(void);
bool		ModelTree_DeleteItem(HTREEITEM hTreeItem);
HTREEITEM	ModelTree_GetRootItem(void);
bool		ModelTree_SetItemText(HTREEITEM hTreeItem, LPCSTR psText);
LPCSTR		ModelTree_GetItemText(HTREEITEM hTreeItem, bool bPure = false);
DWORD		ModelTree_GetItemData(HTREEITEM hTreeItem);
HTREEITEM	ModelTree_GetChildItem(HTREEITEM hTreeItem);
bool		ModelTree_ItemHasChildren(HTREEITEM hTreeItem);
int			ModelTree_GetChildCount(HTREEITEM hTreeItem);
void		ModelTree_InsertSequences(ModelContainer_t *pContainer, HTREEITEM hTreeItem_Sequences);
void		ModelTree_InsertSequences(ModelHandle_t hModel,			HTREEITEM hTreeItem_Sequences);
HTREEITEM	ModelTree_GetNextSiblingItem(HTREEITEM hTreeItem);
HTREEITEM	ModelTree_InsertItem(LPCTSTR psName, HTREEITEM hParent, UINT32 uiUserData = NULL, HTREEITEM hInsertAfter = TVI_LAST);
HTREEITEM	ModelTree_GetRootSurface(ModelHandle_t hModel);
HTREEITEM	ModelTree_GetRootBone(ModelHandle_t hModel);

void R_ModelContainer_Apply(ModelContainer_t* pContainer, void (*pFunction) ( ModelContainer_t* pContainer, void *pvData), void *pvData = NULL);

ModelContainer_t*	ModelContainer_FindFromModelHandle(ModelHandle_t hModel);
int		ModelContainer_BoneIndexFromName(ModelContainer_t *pContainer, LPCSTR psBoneName);

void	Media_Delete(void);
void	AppVars_WriteIdeal(void);
void	AppVars_ReadIdeal(void);

bool	Model_Sequence_Lock		( ModelHandle_t hModel, int iSequenceNumber,   bool bPrimary);
bool	Model_Sequence_Lock		( ModelHandle_t hModel, LPCSTR psSequenceName, bool bPrimary, bool bOktoShowErrorBox = true);
bool	Model_Sequence_UnLock	( ModelHandle_t hModel, bool bPrimary);
bool	Model_Sequence_IsLocked	( ModelHandle_t hModel, int iSequenceNumber, bool bPrimary);
LPCSTR	Model_Sequence_GetName	( ModelHandle_t hModel, int iSequenceNumber, bool bUsedForDisplay = false);
LPCSTR	Model_Sequence_GetName	( ModelContainer_t *pContainer, int iSequenceNumber, bool bUsedForDisplay = false);
LPCSTR	Model_Sequence_GetLockedName( ModelHandle_t hModel, bool bPrimary);
int		Model_Sequence_IndexForName(ModelContainer_t *pContainer, LPCSTR psSeqName);
bool	Model_SecondaryAnimLockingActive(ModelHandle_t hModel);
bool	Model_SecondaryAnimLockingActive(const ModelContainer_t *pContainer);
LPCSTR  Model_Sequence_GetTreeName(ModelHandle_t hModel, int iSequenceNumber);
int		Model_GetG2SurfaceRootOverride	(ModelContainer_t *pContainer);
int		Model_GetG2SurfaceRootOverride	(ModelHandle_t hModel);
bool	Model_SetG2SurfaceRootOverride	(ModelContainer_t *pContainer, int iSurfaceNum);
bool	Model_SetG2SurfaceRootOverride	(ModelContainer_t *pContainer, LPCSTR psSurfaceName);
bool	Model_SetG2SurfaceRootOverride	(ModelHandle_t hModel, int iSurfaceNum);

bool	Model_MultiSeq_Add		( ModelHandle_t hModel, int iSequenceNumber, bool bPrimary, bool bActivate = true);
bool	Model_MultiSeq_IsActive	( ModelHandle_t		hModel,		bool bPrimary);
bool	Model_MultiSeq_IsActive	( ModelContainer_t *pContainer, bool bPrimary);
void	Model_MultiSeq_SetActive( ModelHandle_t hModel, bool bPrimary, bool bActive);
int		Model_MultiSeq_GetNumEntries(ModelHandle_t		hModel,			bool bPrimary);
int		Model_MultiSeq_GetNumEntries(ModelContainer_t	*pContainer,	bool bPrimary);
int		Model_MultiSeq_GetEntry	( ModelContainer_t *pContainer, int iEntry, bool bPrimary);
void	Model_MultiSeq_Clear	( ModelHandle_t hModel, bool bPrimary);
void	Model_MultiSeq_DeleteLast(ModelHandle_t hModel, bool bPrimary);
void	Model_MultiSeq_Delete	( ModelHandle_t hModel, int iSeqIndex, bool bPrimary);
int		Model_MultiSeq_SeqIndexFromFrame(ModelContainer_t *pContainer, int iFrame, bool bPrimary, bool bIsOldFrame );
int		Model_MultiSeq_EntryIndexFromFrame(ModelContainer_t *pContainer, int iFrame, bool bPrimary );
bool	Model_MultiSeq_AlreadyContains(ModelHandle_t hModel, int iSequenceNumber, bool bPrimary);
bool	Model_MultiSeq_AlreadyContains(ModelContainer_t *pContainer, int iSequenceNumber, bool bPrimary);

#endif	// #ifndef MODEL_H


////////////// eof /////////////

