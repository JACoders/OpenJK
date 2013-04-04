// Filename:-	skins.h
//

#ifndef SKINS_H
#define SKINS_H
#include "disablewarnings.h"

/*
class CAlternativeMaterial
{
protected:
	string m_sName;

public:
	CSkinMaterialShader(LPCSTR psName) {m_sName=psName;}
	~CSkinMaterialShader()			{ OutputDebugString( va("~CSkinMaterialShader(): %s\n",m_sName.c_str()) ); }
};
*/
typedef struct
{
	StringVector_t	vSurfacesOn;
	StringVector_t	vSurfacesOff;
	StringVector_t	vSurfacesOffNoChildren;

} SurfaceOnOffPrefs_t;

typedef vector<string/*CAlternativeMaterial*/> AlternativeShaders_t;		// each string = shader name
typedef map<string,AlternativeShaders_t> ShadersForMaterial_t;	// map key = (eg) "face", entry = alternative shader list
typedef map<string,ShadersForMaterial_t> SkinSet_t;				// map key = (eg) "white",entry = body component
typedef map<string,SkinSet_t> SkinSets_t;						// map key = (eg) "thug",entry = skin set

typedef map<string,SurfaceOnOffPrefs_t> SkinSetsSurfacePrefs_t;

bool Skins_ApplyEthnic	( ModelContainer_t *pContainer, LPCSTR psSkin, LPCSTR psEthnic, bool bApplySurfacePrefs, bool bDefaultSurfaces);
bool Skins_ApplySkinShaderVariant(ModelContainer_t *pContainer, LPCSTR psSkin, LPCSTR psEthnic, LPCSTR psMaterial, int iVariant );
bool Skins_Validate		( ModelContainer_t *pContainer, int iSkinNumber );
bool Skins_ApplySkinFile(ModelContainer_t *pContainer, string strSkinFile, string strEthnic, bool bApplySurfacePrefs, bool bDefaultSurfaces, string strMaterial = "", int iVariant = -1);
bool Skins_FilesExist	(LPCSTR psModelFilename);
bool Skins_Read			(LPCSTR psModelFilename, ModelContainer_t *pContainer);
bool Skins_ApplyToTree	(HTREEITEM hTreeItem_Parent, ModelContainer_t *pContainer);
void Skins_ApplyDefault	(ModelContainer_t *pContainer);
bool Skins_FileHasSurfacePrefs(ModelContainer_t *pContainer, LPCSTR psSkin);
void Skins_KillPreCacheInfo(void);
GLuint AnySkin_GetGLBind( ModelHandle_t hModel, LPCSTR psMaterialName, LPCSTR psSurfaceName );


#endif	// #ifndef SKINS_H

//////////////////// eof //////////////////////


