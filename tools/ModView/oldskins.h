// Filename:-	oldskins.h
//


#ifndef OLDSKINS_H
#define OLDSKINS_H


typedef vector< pair<string,string> > StringPairVector_t;
typedef map<string,StringPairVector_t> OldSkinSets_t;	// map key = (eg) "blue", string-pairs


bool OldSkins_FilesExist( LPCSTR psLocalFilename_GLM );
bool OldSkins_Read		( LPCSTR psLocalFilename_GLM, ModelContainer_t *pContainer );
bool OldSkins_Apply		( ModelContainer_t *pContainer, LPCSTR psSkinName );
bool OldSkins_ApplyToTree(HTREEITEM hTreeItem_Parent, ModelContainer_t *pContainer );
bool OldSkins_Validate	( ModelContainer_t *pContainer, int iSkinNumber );
void OldSkins_ApplyDefault(ModelContainer_t *pContainer);
GLuint OldSkins_GetGLBind(ModelContainer_t *pContainer, LPCSTR psSurfaceName);


#endif	// #ifndef OLDSKINS_H

/////////////////// eof /////////////////

