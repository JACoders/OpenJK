// ModViewTreeView.cpp : implementation file
//

#include "stdafx.h"
#include "includes.h"
#include "ModView.h"
#include "GetString.h"

#include "ModViewTreeView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CModViewTreeView* gModViewTreeViewhandle = NULL;

/////////////////////////////////////////////////////////////////////////////
// CModViewTreeView

IMPLEMENT_DYNCREATE(CModViewTreeView, CTreeView)

CModViewTreeView::CModViewTreeView()
{
}

CModViewTreeView::~CModViewTreeView()
{
}


BEGIN_MESSAGE_MAP(CModViewTreeView, CTreeView)
	//{{AFX_MSG_MAP(CModViewTreeView)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(IDM_TREE_MODEL_EXPANDALL, OnTreeModelExpandall)
	ON_COMMAND(IDM_TREE_MODEL_CONTRACTALL, OnTreeModelContractall)
	ON_COMMAND(IDM_GLMSURFACE_INFO, OnGlmsurfaceInfo)
	ON_COMMAND(IDM_TREE_MODEL_INFO, OnTreeModelInfo)
	ON_COMMAND(IDM_GLMSURFACE_OFF, OnGlmsurfaceOff)
	ON_UPDATE_COMMAND_UI(IDM_GLMSURFACE_OFF, OnUpdateGlmsurfaceOff)
	ON_COMMAND(IDM_GLMSURFACE_ON, OnGlmsurfaceOn)
	ON_UPDATE_COMMAND_UI(IDM_GLMSURFACE_ON, OnUpdateGlmsurfaceOn)
	ON_COMMAND(IDM_GLMSURFACE_NODESCENDANTS, OnGlmsurfaceNodescendants)
	ON_UPDATE_COMMAND_UI(IDM_GLMSURFACE_NODESCENDANTS, OnUpdateGlmsurfaceNodescendants)
	ON_COMMAND(ID_SEQ_LOCK, OnSeqLock)
	ON_UPDATE_COMMAND_UI(ID_SEQ_LOCK, OnUpdateSeqLock)
	ON_UPDATE_COMMAND_UI(ID_SEQS_UNLOCKALL, OnUpdateSeqsUnlockall)
	ON_COMMAND(ID_SEQS_UNLOCKALL, OnSeqsUnlockall)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(IDR_TREE_SURFACES_EXPANDALL, OnTreeSurfacesExpandall)
	ON_COMMAND(IDR_TREE_BONES_EXPANDALL, OnTreeBonesExpandall)
	ON_COMMAND(IDM_GLM_BONEBOLT, OnGlmBonebolt)
	ON_COMMAND(IDM_GLM_BONECLEARBOLT, OnGlmBoneclearbolt)
	ON_UPDATE_COMMAND_UI(IDM_GLM_BONECLEARBOLT, OnUpdateGlmBoneclearbolt)
	ON_COMMAND(IDM_GLMBONE_INFO, OnGlmboneInfo)
	ON_COMMAND(IDM_TREE_MODEL_UNBOLTME, OnTreeModelUnboltme)
	ON_UPDATE_COMMAND_UI(IDM_TREE_MODEL_UNBOLTME, OnUpdateTreeModelUnboltme)
	ON_UPDATE_COMMAND_UI(ID_JUNK, OnUpdateJunk)
	ON_UPDATE_COMMAND_UI(IDM_GLMBONE_TITLE, OnUpdateGlmboneTitle)
	ON_UPDATE_COMMAND_UI(IDM_GLMSURFACE_TITLE, OnUpdateGlmsurfaceTitle)
	ON_COMMAND(ID_SEQ_UNLOCK, OnSeqUnlock)
	ON_UPDATE_COMMAND_UI(ID_SEQ_UNLOCK, OnUpdateSeqUnlock)
	ON_UPDATE_COMMAND_UI(ID_SEQ_TITLE, OnUpdateSeqTitle)
	ON_COMMAND(IDM_GLMTAGSURFACE_INFO, OnGlmtagsurfaceInfo)
	ON_COMMAND(IDM_GLMTAGSURFACE_BOLT, OnGlmtagsurfaceBolt)
	ON_COMMAND(IDM_GLMTAGSURFACE_DELETEBOLT, OnGlmtagsurfaceDeletebolt)
	ON_UPDATE_COMMAND_UI(IDM_GLMTAGSURFACE_DELETEBOLT, OnUpdateGlmtagsurfaceDeletebolt)
	ON_COMMAND(IDR_TREE_TAGSURFACES_EXPANDALL, OnTreeTagsurfacesExpandall)
	ON_UPDATE_COMMAND_UI(IDM_GLMTAGSURFACE_TITLE, OnUpdateGlmtagsurfaceTitle)
	ON_COMMAND(IDR_TREE_SURFACES_ALLDEFAULTOFF_ON, OnTreeSurfacesAlldefaultoffOn)
	ON_UPDATE_COMMAND_UI(IDR_TREE_SURFACES_ALLDEFAULTOFF_ON, OnUpdateTreeSurfacesAlldefaultoffOn)
	ON_COMMAND(IDR_TREE_SURFACES_ALLDEFAULTOFF_OFF, OnTreeSurfacesAlldefaultoffOff)
	ON_UPDATE_COMMAND_UI(IDR_TREE_SURFACES_ALLDEFAULTOFF_OFF, OnUpdateTreeSurfacesAlldefaultoffOff)
	ON_COMMAND(IDM_SURFACE_BOLT, OnSurfaceBolt)
	ON_UPDATE_COMMAND_UI(IDM_SURFACE_BOLT, OnUpdateSurfaceBolt)
	ON_COMMAND(ID_SURFACE_DELETEMODELBOLTEDTOTHISSURFACE, OnSurfaceDeletemodelboltedtothissurface)
	ON_UPDATE_COMMAND_UI(ID_SURFACE_DELETEMODELBOLTEDTOTHISSURFACE, OnUpdateSurfaceDeletemodelboltedtothissurface)
	ON_COMMAND(ID_SKINS_VALIDATE, OnSkinsValidate)
	ON_COMMAND(ID_SKIN_VALIDATE, OnSkinValidate)
	ON_COMMAND(ID_EXPANDALL, OnExpandall)
	ON_COMMAND(ID_VARIANT_APPLY, OnVariantApply)
	ON_COMMAND(ID_ETHNIC_APPLY, OnEthnicApply)
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAll)
	ON_COMMAND(ID_SKIN_EXPANDALL, OnSkinExpandall)
	ON_COMMAND(ID_OLDSKINS_VALIDATE, OnOldskinsValidate)
	ON_COMMAND(ID_OLDSKIN_VALIDATE, OnOldskinValidate)
	ON_COMMAND(ID_OLDSKIN_APPLY, OnOldskinApply)
	ON_COMMAND(IDM_GLM_BONE_LOWERANIMSTART, OnGlmBoneLoweranimstart)
	ON_UPDATE_COMMAND_UI(IDM_GLM_BONE_LOWERANIMSTART, OnUpdateGlmBoneLoweranimstart)
	ON_COMMAND(ID_BONES_CLEARSECONDARYANIM, OnBonesClearsecondaryanim)
	ON_UPDATE_COMMAND_UI(ID_BONES_CLEARSECONDARYANIM, OnUpdateBonesClearsecondaryanim)
	ON_COMMAND(ID_SEQ_LOCK_SECONDARY, OnSeqLockSecondary)
	ON_UPDATE_COMMAND_UI(ID_SEQ_LOCK_SECONDARY, OnUpdateSeqLockSecondary)
	ON_COMMAND(ID_SEQ_UNLOCK_SECONDARY, OnSeqUnlockSecondary)
	ON_UPDATE_COMMAND_UI(ID_SEQ_UNLOCK_SECONDARY, OnUpdateSeqUnlockSecondary)
	ON_COMMAND(ID_SEQS_UNLOCK_PRIMARY, OnSeqsUnlockPrimary)
	ON_UPDATE_COMMAND_UI(ID_SEQS_UNLOCK_PRIMARY, OnUpdateSeqsUnlockPrimary)
	ON_COMMAND(ID_SEQS_UNLOCK_SECONDARY, OnSeqsUnlockSecondary)
	ON_UPDATE_COMMAND_UI(ID_SEQS_UNLOCK_SECONDARY, OnUpdateSeqsUnlockSecondary)
	ON_COMMAND(ID_SURFACES_FIND, OnSurfacesFind)
	ON_COMMAND(ID_BONES_FIND, OnBonesFind)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_COMMAND(ID_MODEL_FINDANY, OnModelFindany)
	ON_COMMAND(ID_SEQUENCES_VIEWFULLPATH, OnSequencesViewfullpath)
	ON_UPDATE_COMMAND_UI(ID_ETHNIC_APPLYWITHSURFACES, OnUpdateEthnicApplywithsurfaces)
	ON_COMMAND(ID_ETHNIC_APPLYWITHSURFACES, OnEthnicApplywithsurfaces)
	ON_COMMAND(IDR_TREE_SURFACES_ALLDEFAULTOFF_DEFAULT, OnTreeSurfacesAlldefaultoffDefault)
	ON_COMMAND(ID_ETHNIC_APPLYWITHSURFACEDEFAULTING, OnEthnicApplywithsurfacedefaulting)
	ON_COMMAND(ID_SEQ_MULTILOCK, OnSeqMultilock)
	ON_UPDATE_COMMAND_UI(ID_SEQ_MULTILOCK, OnUpdateSeqMultilock)
	ON_UPDATE_COMMAND_UI(ID_SEQ_MULTILOCK_SECONDARY, OnUpdateSeqMultilockSecondary)
	ON_COMMAND(ID_SEQ_MULTILOCK_SECONDARY, OnSeqMultilockSecondary)
	ON_COMMAND(ID_MULTISEQS_UNLOCK_PRIMARY, OnMultiseqsUnlockPrimary)
	ON_UPDATE_COMMAND_UI(ID_MULTISEQS_UNLOCK_PRIMARY, OnUpdateMultiseqsUnlockPrimary)
	ON_COMMAND(ID_MULTISEQS_UNLOCK_SECONDARY, OnMultiseqsUnlockSecondary)
	ON_UPDATE_COMMAND_UI(ID_MULTISEQS_UNLOCK_SECONDARY, OnUpdateMultiseqsUnlockSecondary)
	ON_COMMAND(ID_SEQS_DELETELAST_PRIMARY, OnSeqsDeletelastPrimary)
	ON_UPDATE_COMMAND_UI(ID_SEQS_DELETELAST_PRIMARY, OnUpdateSeqsDeletelastPrimary)
	ON_COMMAND(ID_SEQS_DELETEALL_PRIMARY, OnSeqsDeleteallPrimary)
	ON_UPDATE_COMMAND_UI(ID_SEQS_DELETEALL_PRIMARY, OnUpdateSeqsDeleteallPrimary)
	ON_COMMAND(ID_SEQS_DELETELAST_SECONDARY, OnSeqsDeletelastSecondary)
	ON_UPDATE_COMMAND_UI(ID_SEQS_DELETELAST_SECONDARY, OnUpdateSeqsDeletelastSecondary)
	ON_COMMAND(ID_SEQS_DELETEALL_SECONDARY, OnSeqsDeleteallSecondary)
	ON_UPDATE_COMMAND_UI(ID_SEQS_DELETEALL_SECONDARY, OnUpdateSeqsDeleteallSecondary)
	ON_COMMAND(ID_SEQ_MULTILOCK_DELETE, OnSeqMultilockDelete)
	ON_UPDATE_COMMAND_UI(ID_SEQ_MULTILOCK_DELETE, OnUpdateSeqMultilockDelete)
	ON_COMMAND(ID_SEQ_MULTILOCK_SECONDARY_DELETE, OnSeqMultilockSecondaryDelete)
	ON_UPDATE_COMMAND_UI(ID_SEQ_MULTILOCK_SECONDARY_DELETE, OnUpdateSeqMultilockSecondaryDelete)
	ON_COMMAND(IDM_GLMSURFACE_SETASROOT, OnGlmsurfaceSetasroot)
	ON_UPDATE_COMMAND_UI(IDM_GLMSURFACE_SETASROOT, OnUpdateGlmsurfaceSetasroot)
	ON_COMMAND(IDR_TREE_SURFACES_CLEARROOT, OnTreeSurfacesClearroot)
	ON_UPDATE_COMMAND_UI(IDR_TREE_SURFACES_CLEARROOT, OnUpdateTreeSurfacesClearroot)
	ON_COMMAND(IDM_GLM_ADDBONEBOLT, OnGlmAddbonebolt)
	ON_COMMAND(IDM_SURFACE_ADDBOLT, OnSurfaceAddbolt)
	ON_COMMAND(IDM_GLMTAGSURFACE_ADDBOLT, OnGlmtagsurfaceAddbolt)
	ON_UPDATE_COMMAND_UI(IDM_SURFACE_ADDBOLT, OnUpdateSurfaceAddbolt)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_SEQUENCES_SORTALPHABETICALLY, OnSequencesSortalphabetically)
	ON_UPDATE_COMMAND_UI(ID_SEQUENCES_SORTALPHABETICALLY, OnUpdateSequencesSortalphabetically)
	ON_COMMAND(IDM_GLMSURFACE_CLEARROOT, OnGlmsurfaceClearroot)
	ON_UPDATE_COMMAND_UI(IDM_GLMSURFACE_CLEARROOT, OnUpdateGlmsurfaceClearroot)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModViewTreeView drawing

//DEL void CModViewTreeView::OnDraw(CDC* pDC)
//DEL {
//DEL 	CDocument* pDoc = GetDocument();
//DEL 	// TODO: add draw code here
//DEL }

/////////////////////////////////////////////////////////////////////////////
// CModViewTreeView diagnostics

#ifdef _DEBUG
void CModViewTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CModViewTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CModViewTreeView message handlers


BOOL CModViewTreeView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CTreeView::PreCreateWindow(cs))
		return FALSE;

	cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;
	return TRUE;
}


BOOL CModViewTreeView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	BOOL b = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	gModViewTreeViewhandle = this;


//	HTREEITEM h =	InsertItem("test_root_item", NULL);
//					InsertItem("child", h);

	return b;
}


HTREEITEM CModViewTreeView::GetRootItem(void)
{
	return GetTreeCtrl().GetRootItem();
}


// hParent can be NULL...
//
HTREEITEM CModViewTreeView::InsertItem(LPCTSTR psName, HTREEITEM hParent, UINT32 uiUserData, HTREEITEM hInsertAfter)
{
	if (hParent == NULL)
	{
		hParent = TVI_ROOT;
	}

	HTREEITEM hTreeItem = GetTreeCtrl().InsertItem(psName, hParent, hInsertAfter);
	assert(hTreeItem);
	GetTreeCtrl().SetItemData(hTreeItem, uiUserData);
	return hTreeItem;
}


BOOL CModViewTreeView::DeleteAllItems()
{
	return GetTreeCtrl().DeleteAllItems();
}



void CModViewTreeView::PostNcDestroy() 
{
	gModViewTreeViewhandle = NULL;	// tell rest of code not to bother trying to write to this now
	
	CTreeView::PostNcDestroy();
}


void CModViewTreeView::UpdateUI(CMenu* pMenu)
{
	CCmdUI state;
	state.m_pMenu = pMenu;
	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pParentMenu == NULL);

	state.m_nIndexMax = pMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
	  state.m_nIndex++)
	{
		state.m_nID = pMenu->GetMenuItemID(state.m_nIndex);
		if (state.m_nID == 0)
			continue; // menu separator or invalid cmd - ignore it

		ASSERT(state.m_pOther == NULL);
		ASSERT(state.m_pMenu != NULL);
		if (state.m_nID == (UINT)-1)
		{
			// possibly a popup menu, route to first item of that popup
			state.m_pSubMenu = pMenu->GetSubMenu(state.m_nIndex);
			if (state.m_pSubMenu == NULL ||
				(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
				state.m_nID == (UINT)-1)
			{
				continue;       // first item of popup can't be routed to
			}
			state.DoUpdate(this, FALSE);    // popups are never auto disabled
		}
		else
		{
			// normal menu item
			state.m_pSubMenu = NULL;
			state.DoUpdate(this, state.m_nID < 0xF000);
		}

		// adjust for menu deletions and additions
		UINT nCount = pMenu->GetMenuItemCount();
		if (nCount < state.m_nIndexMax)
		{
			state.m_nIndex -= (state.m_nIndexMax - nCount);
			while (state.m_nIndex < nCount &&
				pMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
			{
				state.m_nIndex++;
			}
		}
		state.m_nIndexMax = nCount;
	}
}



HTREEITEM		ghTreeItem_RButtonMenu = NULL;	// rather tacky, but I blame MS's API weakness...
TreeItemData_t	gTreeItemData;//UINT32		uiUserData_RButtonMenu;
void CModViewTreeView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	UINT nHitFlags = 0;
	HTREEITEM hTreeItem_Clicked = GetTreeCtrl().HitTest( point, &nHitFlags );

/*
	if (hTreeItem_Clicked != NULL)
	{
		BOOL bSelect = (GetTreeCtrl().GetItemState(hTreeItem_Clicked, TVIS_SELECTED) & TVIS_SELECTED) != 0;
		if (!bSelect)
		{
//			GetTreeCtrl().ClearSelection();
//			GetTreeCtrl().SelectItem( hTreeItem_Clicked );
//			GetTreeCtrl().SetItemState( hTreeItem_Clicked, TVIS_SELECTED, TVIS_SELECTED );
		}
	}
	else
	{
//		int count = GetSelectedCount();
//		if (count == 0)
		{
			CMenu theMenu;
			theMenu.LoadMenu(IDR_TREEPOPUP_MODEL);
			CMenu* thePopup = theMenu.GetSubMenu(0);

			// ignore this, do the simple stuff instead since the menus (as yet) contain no r/t additions...
//			UpdateUI(thePopup);
//			ShowPopup(point, thePopup);
				CPoint clientPoint = point;
				ClientToScreen(&clientPoint);
				thePopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, clientPoint.x, clientPoint.y, this);

			theMenu.DestroyMenu();
			return;
		}

	}
//	GetTreeCtrl().SetItemData(

	// TODO: Add your message handler code here and/or call default
*/

	UINT	nIDMenuResource	= NULL;
	TreeItemData_t	TreeItemData={0};

	if (!hTreeItem_Clicked)	// if we didn't physically click on something, see if there's anything selected
	{
		hTreeItem_Clicked = GetTreeCtrl().GetSelectedItem();
	}

	if (hTreeItem_Clicked)
	{
		TreeItemData.uiData = GetTreeCtrl().GetItemData(hTreeItem_Clicked);
	}
	else
	{
		hTreeItem_Clicked = GetTreeCtrl().GetRootItem();
		TreeItemData.iItemType		= TREEITEMTYPE_MODELNAME;
		TreeItemData.iModelHandle	= Model_GetPrimaryHandle();	// default to primary if not clicked on something specific
	}

	switch (TreeItemData.iItemType)
	{
		case TREEITEMTYPE_MODELNAME:	
			
			nIDMenuResource = IDR_TREEPOPUP_MODEL;
			break;

		case TREEITEMTYPE_SURFACEHEADER:	// "surfaces"

			nIDMenuResource = IDR_TREEPOPUP_SURFACES;
			break;

		case TREEITEMTYPE_TAGSURFACEHEADER:	// "surfaces"

			nIDMenuResource = IDR_TREEPOPUP_TAGSURFACES;
			break;

		case TREEITEMTYPE_BONEHEADER:		// "bones"

			nIDMenuResource = IDR_TREEPOPUP_BONES;
			break;

		case TREEITEMTYPE_SEQUENCEHEADER:	// "sequences"

			nIDMenuResource = IDR_TREEPOPUP_SEQUENCES;
			break;

		case TREEITEMTYPE_GLM_SURFACE:

			nIDMenuResource = IDR_TREEPOPUP_GLMSURFACE;
			break;

		case TREEITEMTYPE_GLM_TAGSURFACE:

			nIDMenuResource = IDR_TREEPOPUP_GLMTAGSURFACE;
			break;

		case TREEITEMTYPE_GLM_BONE:
		case TREEITEMTYPE_GLM_BONEALIAS:	// not sure about this one...

			nIDMenuResource = IDR_TREEPOPUP_GLMBONE;
			break;

		case TREEITEMTYPE_SEQUENCE:

			nIDMenuResource = IDR_TREEPOPUP_SEQUENCE;
			break;			

		case TREEITEMTYPE_SKINSHEADER:

			nIDMenuResource = IDR_TREEPOPUP_SKINS;
			break;			

		case TREEITEMTYPE_OLDSKINSHEADER:

			nIDMenuResource = IDR_TREEPOPUP_OLDSKINS;
			break;

		case TREEITEMTYPE_SKIN:

			nIDMenuResource = IDR_TREEPOPUP_SKIN;
			break;			

		case TREEITEMTYPE_OLDSKIN:

			nIDMenuResource = IDR_TREEPOPUP_OLDSKIN;
			break;			

		case TREEITEMTYPE_SKINETHNIC:

			nIDMenuResource = IDR_TREEPOPUP_ETHNIC;
			break;
			
		case TREEITEMTYPE_SKINMATERIALSHADER:

			nIDMenuResource = IDR_TREEPOPUP_SHADERVARIANT;
			break;			
  	}

	// do a popup?
	//
	if (nIDMenuResource)
	{
		// record globally for menu code to acces (tacky, I know...)
		//
		ghTreeItem_RButtonMenu	= hTreeItem_Clicked;
		gTreeItemData			= TreeItemData;	//		uiUserData_RButtonMenu= TreeItemData.uiData;	//uiTreeItemData;

		if (gTreeItemData.iModelHandle)	// don't do anything if no model loaded
		{
			CMenu theMenu;
			theMenu.LoadMenu( nIDMenuResource );			
			CMenu* thePopup = theMenu.GetSubMenu(0);

			UpdateUI(thePopup);
/*
			AfxGetApp()->OnIdle(1);

----------
		UINT nCount = pMenu->GetMenuItemCount();
		if (nCount < state.m_nIndexMax)
		{
			state.m_nIndex -= (state.m_nIndexMax - nCount);
			while (state.m_nIndex < nCount &&
				pMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
			{
				state.m_nIndex++;
			}
		}
		state.m_nIndexMax = nCount;

-----------
*/
			CPoint clientPoint = point;
			ClientToScreen(&clientPoint);
			thePopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, clientPoint.x, clientPoint.y, this);

			theMenu.DestroyMenu();
			return;
		}
	}

	CTreeView::OnRButtonDown(nFlags, point);
}




// these would ideally be member functions, but there's too much __cdecl* crap because of fastcall members so you can't
//	pass member functions ptrs on the stack under this build. Sigh...
//
static void ExpandTreeItem( HTREEITEM hTreeItem )
{
	gModViewTreeViewhandle->GetTreeCtrl().Expand( hTreeItem, TVE_EXPAND );
}

static void CollapseTreeItem( HTREEITEM hTreeItem )
{
	gModViewTreeViewhandle->GetTreeCtrl().Expand( hTreeItem, TVE_COLLAPSE );
}

// 'bDefaultAll' overrides 'bOnOff' and means set-to-default-state...
//
static void SetTreeItemSurfaceState( HTREEITEM hTreeItem, bool bOnOff, bool bDefaultAll )
{
	TreeItemData_t	TreeItemData;
					TreeItemData.uiData = gModViewTreeViewhandle->GetTreeCtrl().GetItemData(hTreeItem);

	if (	TreeItemData.iItemType	== TREEITEMTYPE_GLM_SURFACE ||
			TreeItemData.iItemType	== TREEITEMTYPE_GLM_TAGSURFACE
		)
	{
		LPCSTR psSurfaceName = GLMModel_GetSurfaceName( TreeItemData.iModelHandle, TreeItemData.iItemNumber );

		bool bSurfaceNameIncludesOFF = !stricmp("_off", &psSurfaceName[strlen(psSurfaceName)-4]);

		if (bDefaultAll || bSurfaceNameIncludesOFF)
		{
			if (bDefaultAll)
				bOnOff = !bSurfaceNameIncludesOFF;

			if (bOnOff)
			{
				Model_GLMSurface_On(TreeItemData.iModelHandle, TreeItemData.iItemNumber);
			}
			else
			{
				Model_GLMSurface_Off(TreeItemData.iModelHandle, TreeItemData.iItemNumber);
			}
		}
	}
}

static void EnableTreeItemDefaultOFFSurface( HTREEITEM hTreeItem )
{
	SetTreeItemSurfaceState( hTreeItem, true,  false );
}

static void DisableTreeItemDefaultOFFSurface( HTREEITEM hTreeItem )
{
	SetTreeItemSurfaceState( hTreeItem, false, false );
}

static void DefaultTreeItemDefaultOFFSurface( HTREEITEM hTreeItem )
{
	SetTreeItemSurfaceState( hTreeItem, false, true );
}


void CModViewTreeView::R_ApplyToTreeItem( void (*pFunction) ( HTREEITEM hTreeItem ), HTREEITEM hTreeItem, bool bProcessSiblings /* = false */, bool bSkipProcessingOfInitialItem /* = false */)
{
	if (hTreeItem)
	{
		if (!bSkipProcessingOfInitialItem)	// only has meaning for first item, inherently false from then on
		{
			// process item...
			//
			pFunction(hTreeItem);
		}

		// recurse child...
		//		
		R_ApplyToTreeItem( pFunction, GetTreeCtrl().GetChildItem( hTreeItem ), true, false );

		if (bProcessSiblings)	// test only has meaning for entry arg, all others inherently true
		{
			// recurse sibling...
			//
			R_ApplyToTreeItem( pFunction, GetTreeCtrl().GetNextSiblingItem( hTreeItem ), bProcessSiblings, false );
		}
	}
}

void CModViewTreeView::OnTreeModelExpandall() 
{
//	R_ApplyToTreeItem( ::ExpandTreeItem, GetTreeCtrl().GetRootItem(), true );
//	GetTreeCtrl().SelectSetFirstVisible(GetRootItem());
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);
}

void CModViewTreeView::OnTreeModelContractall() 
{
//	R_ApplyToTreeItem( ::CollapseTreeItem, GetTreeCtrl().GetRootItem(), true );
	R_ApplyToTreeItem( ::CollapseTreeItem, ghTreeItem_RButtonMenu, true );
}




void CModViewTreeView::OnTreeModelInfo() 
{
	InfoBox( Model_Info(gTreeItemData.iModelHandle) );	
}


void CModViewTreeView::OnGlmsurfaceOff() 
{		
	Model_GLMSurface_Off(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber);
}

void CModViewTreeView::OnUpdateGlmsurfaceOff(CCmdUI* pCmdUI) 
{
	SurfaceOnOff_t eOnOff = Model_GLMSurface_GetStatus( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber );
	pCmdUI->Enable( !(eOnOff == SURF_OFF || eOnOff == SURF_INHERENTLYOFF) );
}

void CModViewTreeView::OnGlmsurfaceOn() 
{
	Model_GLMSurface_On(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber);	
}

void CModViewTreeView::OnUpdateGlmsurfaceOn(CCmdUI* pCmdUI) 
{
	SurfaceOnOff_t eOnOff = Model_GLMSurface_GetStatus( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber );
	pCmdUI->Enable( !(eOnOff == SURF_ON || eOnOff == SURF_INHERENTLYOFF) );
}

void CModViewTreeView::OnGlmsurfaceNodescendants() 
{
	Model_GLMSurface_NoDescendants(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber);	
}

void CModViewTreeView::OnUpdateGlmsurfaceNodescendants(CCmdUI* pCmdUI) 
{
	SurfaceOnOff_t eOnOff = Model_GLMSurface_GetStatus( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber );
	pCmdUI->Enable( !(eOnOff == SURF_NO_DESCENDANTS || eOnOff == SURF_INHERENTLYOFF) );
}

void CModViewTreeView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
//	OutputDebugString("OnSelchanged\n");
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	HTREEITEM hTreeItem = GetTreeCtrl().GetSelectedItem();

	TreeItemData_t	TreeItemData;
					TreeItemData.uiData = GetTreeCtrl().GetItemData(hTreeItem);

	// for now, clicking on anything will default it all according to switch-case,
	//	so clear everything (eg clicking on a bone will clear all surfaces)
	//
	Model_SetBoneHighlight		(TreeItemData.iModelHandle, iITEMHIGHLIGHT_NONE);
	Model_SetSurfaceHighlight	(TreeItemData.iModelHandle, iITEMHIGHLIGHT_NONE);

	switch (TreeItemData.iItemType)
	{
		case TREEITEMTYPE_BONEHEADER:		// "bones" tree header			

			Model_SetBoneHighlight(TreeItemData.iModelHandle, iITEMHIGHLIGHT_ALL);
			break;

		case TREEITEMTYPE_BONEALIASHEADER:	// "bone aliases" tree header

			Model_SetBoneHighlight(TreeItemData.iModelHandle, iITEMHIGHLIGHT_ALIASED);
			break;

		case TREEITEMTYPE_GLM_BONE:			// individual bone
		case TREEITEMTYPE_GLM_BONEALIAS:	// (or an alias)

			Model_SetBoneHighlight(TreeItemData.iModelHandle, TreeItemData.iItemNumber);
			break;

		case TREEITEMTYPE_SURFACEHEADER:	// "surfaces" tree header		

			Model_SetSurfaceHighlight(TreeItemData.iModelHandle, iITEMHIGHLIGHT_ALL);
			break;

		case TREEITEMTYPE_TAGSURFACEHEADER:	// "tagsurfaces" tree header

			Model_SetSurfaceHighlight(TreeItemData.iModelHandle, iITEMHIGHLIGHT_ALL_TAGSURFACES);
			break;

		// this case would probably be for all types of surface (in this one spot)...
		//
		case TREEITEMTYPE_GLM_SURFACE:		// individual surface
		case TREEITEMTYPE_GLM_TAGSURFACE:	// individual surface, albeit a tag

			Model_SetSurfaceHighlight(TreeItemData.iModelHandle, TreeItemData.iItemNumber);
			break;

		default:	// selecting anything else will un-highlight all bones

			// Model_SetBoneHighlight		(TreeItemData.iModelHandle, iITEMHIGHLIGHT_NONE);
			// Model_SetSurfaceHighlight	(TreeItemData.iModelHandle, iITEMHIGHLIGHT_NONE);
			break;
	}
	
	*pResult = 0;

	// may as well copy to global ones...
	//
	gTreeItemData.uiData = TreeItemData.uiData;
	ghTreeItem_RButtonMenu = hTreeItem;
}


void CModViewTreeView::OnTreeSurfacesExpandall()
{
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);
}

void CModViewTreeView::OnTreeBonesExpandall()
{
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);
}


void CModViewTreeView::OnGlmBonebolt() 
{
//	OutputDebugString("on command\n");
	LPCSTR psCaption = va("Bolt model to bonebolt-point '%s'",Model_GetBoltName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true ));	// bBoltIsBone

	LPCSTR psFullPathedFilename = InputLoadFileName("",				// LPCSTR psInitialLoadName, 
													psCaption,		// LPCSTR psCaption,
													Filename_PathOnly(Model_GetFullPrimaryFilename()),	//	"S:\\baseq3\\models\\test\\bonehier",	// LPCSTR psInitialDir,
													Model_GetSupportedTypesFilter()			// LPCSTR psFilter
													);

	if (psFullPathedFilename)
	{
		Model_LoadBoltOn(psFullPathedFilename, gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true, true);	// bBoltIsBone, bBoltReplacesAllExisting
	}
}

void CModViewTreeView::OnGlmtagsurfaceAddbolt() 
{
//	OutputDebugString("on command\n");
	LPCSTR psCaption = va("Bolt additional model to surface-tag '%s'",Model_GetBoltName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false ));	// bBoltIsBone

	LPCSTR psFullPathedFilename = InputLoadFileName("",				// LPCSTR psInitialLoadName, 
													psCaption,		// LPCSTR psCaption,
													Filename_PathOnly(Model_GetFullPrimaryFilename()),	//	"S:\\baseq3\\models\\test\\bonehier",	// LPCSTR psInitialDir,
													Model_GetSupportedTypesFilter()			// LPCSTR psFilter
													);

	if (psFullPathedFilename)
	{
		Model_LoadBoltOn(psFullPathedFilename, gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false, false);	// bBoltIsBone, bBoltReplacesAllExisting
	}
}


void CModViewTreeView::OnSurfaceAddbolt() 
{
	LPCSTR psCaption = va("Bolt additional model to surface-tag '%s'",Model_GetBoltName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false ));	// bBoltIsBone

	LPCSTR psFullPathedFilename = InputLoadFileName("",				// LPCSTR psInitialLoadName, 
													psCaption,		// LPCSTR psCaption,
													Filename_PathOnly(Model_GetFullPrimaryFilename()),	//	"S:\\baseq3\\models\\test\\bonehier",	// LPCSTR psInitialDir,
													Model_GetSupportedTypesFilter()			// LPCSTR psFilter
													);

	if (psFullPathedFilename)
	{
		Model_LoadBoltOn(psFullPathedFilename, gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false, false);	// bBoltIsBone, bBoltReplacesAllExisting
	}
}



void CModViewTreeView::OnGlmAddbonebolt() 
{
//	OutputDebugString("on command\n");
	LPCSTR psCaption = va("Bolt additional model to bonebolt-point '%s'",Model_GetBoltName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true ));	// bBoltIsBone

	LPCSTR psFullPathedFilename = InputLoadFileName("",				// LPCSTR psInitialLoadName, 
													psCaption,		// LPCSTR psCaption,
													Filename_PathOnly(Model_GetFullPrimaryFilename()),	//	"S:\\baseq3\\models\\test\\bonehier",	// LPCSTR psInitialDir,
													Model_GetSupportedTypesFilter()			// LPCSTR psFilter
													);

	if (psFullPathedFilename)
	{
		Model_LoadBoltOn(psFullPathedFilename, gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true, false);	// bBoltIsBone, bBoltReplacesAllExisting
	}
}


void CModViewTreeView::OnGlmBoneclearbolt() 
{		
	if (Model_DeleteBoltOn(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true, -1))
	{
		Invalidate(false);	// or some changed items on the tree don't redraw until you click on them
	}
}

void CModViewTreeView::OnUpdateGlmBoneclearbolt(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(Model_CountItemsBoltedHere(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true));	// bBoltIsBone
}

void CModViewTreeView::OnGlmsurfaceInfo() 
{
	if (AppVars.bVertIndexes)
	{
		string	strInfo = Model_GLMSurfaceInfo( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false);
				strInfo+= "\nSince you have display-vert-indexes on, do you want to see all vert data as well?\n";
		if (GetYesNo( strInfo.c_str() ))
		{
			CWaitCursor wait;
			LPCSTR psInfoString = Model_GLMSurfaceVertInfo( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber );

			SendStringToNotepad(psInfoString, va("%s_%s_vertinfo.txt",Filename_WithoutPath(Filename_WithoutExt(Model_GetFilename(gTreeItemData.iModelHandle))),String_RemoveOccurences(Model_GetSurfaceName(gTreeItemData.iModelHandle,gTreeItemData.iItemNumber),"*")));
		}
	}
	else
	{
		InfoBox( Model_GLMSurfaceInfo( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false));	// bShortVersionForTag
	}
}

void CModViewTreeView::OnGlmboneInfo() 
{
	InfoBox( Model_GLMBoneInfo( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber));
}

void CModViewTreeView::OnTreeModelUnboltme() 
{
	if (Model_DeleteBoltOn(gTreeItemData.iModelHandle))
	{
		Invalidate(false);
	}
}

void CModViewTreeView::OnUpdateTreeModelUnboltme(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(Model_HasParent(gTreeItemData.iModelHandle));
}

void CModViewTreeView::OnUpdateJunk(CCmdUI* pCmdUI) 
{
	// not actually used now...
}

void CModViewTreeView::OnUpdateGlmboneTitle(CCmdUI* pCmdUI)
{
	pCmdUI->SetText(va("Bone:  %s",Model_GetBoneName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber )));
}

void CModViewTreeView::OnUpdateGlmsurfaceTitle(CCmdUI* pCmdUI) 
{
	pCmdUI->SetText(va("Surface:  %s",Model_GetSurfaceName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber )));
}


void CModViewTreeView::OnUpdateSeqTitle(CCmdUI* pCmdUI) 
{
	pCmdUI->SetText(va("Sequence:  %s",Model_Sequence_GetName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber )));
}


void CModViewTreeView::OnTreeTagsurfacesExpandall() 
{
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);	
}

void CModViewTreeView::OnUpdateGlmtagsurfaceTitle(CCmdUI* pCmdUI) 
{
	pCmdUI->SetText(va("Tag Surface:  %s",Model_GetSurfaceName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber )));	
}

void CModViewTreeView::OnGlmtagsurfaceInfo() 
{
	InfoBox( Model_GLMSurfaceInfo( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true));	// bShortVersionForTag
}

void CModViewTreeView::OnGlmtagsurfaceBolt() 
{
//	OutputDebugString("on command\n");
	LPCSTR psCaption = va("Bolt model to surface-tag '%s'",Model_GetBoltName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false ));	// bBoltIsBone

	LPCSTR psFullPathedFilename = InputLoadFileName("",				// LPCSTR psInitialLoadName, 
													psCaption,		// LPCSTR psCaption,
													Filename_PathOnly(Model_GetFullPrimaryFilename()),	//	"S:\\baseq3\\models\\test\\bonehier",	// LPCSTR psInitialDir,
													Model_GetSupportedTypesFilter()			// LPCSTR psFilter
													);

	if (psFullPathedFilename)
	{
		Model_LoadBoltOn(psFullPathedFilename, gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false, true);	// bBoltIsBone, bBoltReplacesAllExisting
	}
}

void CModViewTreeView::OnGlmtagsurfaceDeletebolt() 
{
	if (Model_DeleteBoltOn(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false, -1))
	{
		Invalidate(false);	// or some changed items on the tree don't redraw until you click on them
	}
}

void CModViewTreeView::OnUpdateGlmtagsurfaceDeletebolt(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(Model_CountItemsBoltedHere(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false));	// bBoltIsBone
}

void CModViewTreeView::OnTreeSurfacesAlldefaultoffOn() 
{		
	CWaitCursor wait;	// this function takes a while, because it loop-calls a function that normally only
						//	gets called occasionally, and re-evaluates the tree text

	R_ApplyToTreeItem( ::EnableTreeItemDefaultOFFSurface, ghTreeItem_RButtonMenu );
	Invalidate(false);
}

void CModViewTreeView::OnUpdateTreeSurfacesAlldefaultoffOn(CCmdUI* pCmdUI) 
{
	// not sure if this is worth filling in, maybe later
}

void CModViewTreeView::OnTreeSurfacesAlldefaultoffOff() 
{
	CWaitCursor wait;	// this function takes a while, because it loop-calls a function that normally only
						//	gets called occasionally, and re-evaluates the tree text

	R_ApplyToTreeItem( ::DisableTreeItemDefaultOFFSurface, ghTreeItem_RButtonMenu );
	Invalidate(false);
}

void CModViewTreeView::OnUpdateTreeSurfacesAlldefaultoffOff(CCmdUI* pCmdUI) 
{
	// not sure if this is worth filling in, maybe later
}


void CModViewTreeView::OnTreeSurfacesAlldefaultoffDefault() 
{
	CWaitCursor wait;	// this function takes a while, because it loop-calls a function that normally only
						//	gets called occasionally, and re-evaluates the tree text

	R_ApplyToTreeItem( ::DefaultTreeItemDefaultOFFSurface, ghTreeItem_RButtonMenu );	
	Invalidate(false);
}


// this option is only available for tag surfaces, even though it's for the standard surrface popup
//
void CModViewTreeView::OnSurfaceBolt() 
{
	LPCSTR psCaption = va("Bolt model to surface-tag '%s'",Model_GetBoltName( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false ));	// bBoltIsBone

	LPCSTR psFullPathedFilename = InputLoadFileName("",				// LPCSTR psInitialLoadName, 
													psCaption,		// LPCSTR psCaption,
													Filename_PathOnly(Model_GetFullPrimaryFilename()),	//	"S:\\baseq3\\models\\test\\bonehier",	// LPCSTR psInitialDir,
													Model_GetSupportedTypesFilter()			// LPCSTR psFilter
													);

	if (psFullPathedFilename)
	{
		Model_LoadBoltOn(psFullPathedFilename, gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false, true);	// bBoltIsBone, bBoltReplacesAllExisting
	}
}

// only enable this menu option if this surface is a tag...
//
void CModViewTreeView::OnUpdateSurfaceBolt(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_SurfaceIsTag(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber) );
}


void CModViewTreeView::OnUpdateSurfaceAddbolt(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_SurfaceIsTag(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber) );
}


// this option is only available for tag surfaces, even though it's for the standard surrface popup
//
void CModViewTreeView::OnSurfaceDeletemodelboltedtothissurface() 
{
	if (Model_DeleteBoltOn(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false, -1))	// bBoltIsBone
	{
		Invalidate(false);	// or some changed items on the tree don't redraw until you click on them
	}
}

// only enable this menu option if this surface is a tag, and has something bolted to it...
///
void CModViewTreeView::OnUpdateSurfaceDeletemodelboltedtothissurface(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(	Model_SurfaceIsTag(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber)
					&&
					Model_CountItemsBoltedHere(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false)
					);
}



void CModViewTreeView::OnSkinsValidate() 
{
	Model_ValidateSkin( gTreeItemData.iModelHandle, -1 );
}

void CModViewTreeView::OnSkinValidate() 
{
	Model_ValidateSkin( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber );	
}

void CModViewTreeView::OnExpandall() 
{
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);	
}

void CModViewTreeView::OnEthnicApply() 
{
	HTREEITEM hTreeItemParent = GetTreeCtrl().GetParentItem(ghTreeItem_RButtonMenu);
	CString strParentSkin = GetTreeCtrl().GetItemText(hTreeItemParent);	
	CString strThisEthnic = GetTreeCtrl().GetItemText(ghTreeItem_RButtonMenu);	
	
	Model_ApplyEthnicSkin( gTreeItemData.iModelHandle, strParentSkin, strThisEthnic, false, false );
}

void CModViewTreeView::OnVariantApply() 
{
	HTREEITEM hTreeItemMaterial = GetTreeCtrl().GetParentItem(ghTreeItem_RButtonMenu);
	HTREEITEM hTreeItemEthnic	= GetTreeCtrl().GetParentItem(hTreeItemMaterial);
	HTREEITEM hTreeItemSkin		= GetTreeCtrl().GetParentItem(hTreeItemEthnic);

	CString strMaterial	(GetTreeCtrl().GetItemText(hTreeItemMaterial));
	CString strEthnic	(GetTreeCtrl().GetItemText(hTreeItemEthnic));
	CString strSkin		(GetTreeCtrl().GetItemText(hTreeItemSkin));

	int iVariant = gTreeItemData.iItemNumber;

	Model_ApplySkinShaderVariant( gTreeItemData.iModelHandle, strSkin, strEthnic, strMaterial, iVariant );
}

void CModViewTreeView::OnExpandAll() 
{
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);	
}

void CModViewTreeView::OnSkinExpandall() 
{
	R_ApplyToTreeItem( ::ExpandTreeItem, ghTreeItem_RButtonMenu );
	GetTreeCtrl().SelectSetFirstVisible(ghTreeItem_RButtonMenu);	
}

void CModViewTreeView::OnOldskinsValidate() 
{
	Model_ValidateSkin( gTreeItemData.iModelHandle, -1 );	
}

void CModViewTreeView::OnOldskinValidate() 
{
	Model_ValidateSkin( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber );
}

void CModViewTreeView::OnOldskinApply() 
{
	CString strSkin(GetTreeCtrl().GetItemText(ghTreeItem_RButtonMenu));
	Model_ApplyOldSkin( gTreeItemData.iModelHandle, strSkin );	
}

// secondary anim start, not lower, but can't be bothered changing function names and resource.h etc
//
void CModViewTreeView::OnGlmBoneLoweranimstart() 
{
//	InfoBox("Ignore this for now, Under construction");
	
	Model_SetSecondaryAnimStart(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber);
}

// secondary anim start, not lower, but can't be bothered changing function names and resource.h etc
//
void CModViewTreeView::OnUpdateGlmBoneLoweranimstart(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(	gTreeItemData.iItemNumber != 0	// it's probably bad to be able to set first bone as secondary anim start?
					&&
					!(Model_GetSecondaryAnimStart(gTreeItemData.iModelHandle) == gTreeItemData.iItemNumber)
					);
}

void CModViewTreeView::OnBonesClearsecondaryanim() 
{
	Model_SetSecondaryAnimStart(gTreeItemData.iModelHandle, -1);
}

void CModViewTreeView::OnUpdateBonesClearsecondaryanim(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( (Model_GetSecondaryAnimStart(gTreeItemData.iModelHandle)!=-1));
}

void CModViewTreeView::OnSeqLock() 
{
	Model_Sequence_Lock(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true);
}

void CModViewTreeView::OnUpdateSeqLock(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!Model_Sequence_IsLocked( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true )
					&&
					!Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true)
					);
}

void CModViewTreeView::OnUpdateSeqsUnlockall(CCmdUI* pCmdUI) 
{		
	pCmdUI->Enable(	!Model_Sequence_IsLocked( gTreeItemData.iModelHandle, -1, true )
					||
					(
						Model_SecondaryAnimLockingActive(gTreeItemData.iModelHandle)
						&&
						!Model_Sequence_IsLocked( gTreeItemData.iModelHandle, -1, false )
					)
					||
					Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true)
					||
					Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false)
					);
}

void CModViewTreeView::OnSeqsUnlockall() 
{
	Model_Sequence_UnLock(gTreeItemData.iModelHandle, true);
	Model_Sequence_UnLock(gTreeItemData.iModelHandle, false);
	Model_MultiSeq_SetActive(gTreeItemData.iModelHandle, true, false);
	Model_MultiSeq_SetActive(gTreeItemData.iModelHandle, false,false);
}

void CModViewTreeView::OnSeqUnlock() 
{
	Model_Sequence_UnLock(gTreeItemData.iModelHandle, true);
}

void CModViewTreeView::OnUpdateSeqUnlock(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(	Model_Sequence_IsLocked( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true )
					&&
					!Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true)
					);
}

void CModViewTreeView::OnSeqLockSecondary() 
{
	Model_Sequence_Lock(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false);
}

void CModViewTreeView::OnUpdateSeqLockSecondary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(Model_SecondaryAnimLockingActive(gTreeItemData.iModelHandle)
					&&
					!Model_Sequence_IsLocked( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false )
					&&
					!Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false)
					);
}

void CModViewTreeView::OnSeqUnlockSecondary() 
{
	Model_Sequence_UnLock(gTreeItemData.iModelHandle, false);	
}
	 
void CModViewTreeView::OnUpdateSeqUnlockSecondary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(Model_SecondaryAnimLockingActive(gTreeItemData.iModelHandle)
					&&
					Model_Sequence_IsLocked( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false )
					&&
					!Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false)
					);
}

void CModViewTreeView::OnSeqsUnlockPrimary() 
{
	Model_Sequence_UnLock(gTreeItemData.iModelHandle, true);	
}

void CModViewTreeView::OnUpdateSeqsUnlockPrimary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!Model_Sequence_IsLocked( gTreeItemData.iModelHandle, -1, true )
					&&
					!Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true)
					);
}

void CModViewTreeView::OnSeqsUnlockSecondary() 
{
	Model_Sequence_UnLock(gTreeItemData.iModelHandle, false);
}

void CModViewTreeView::OnUpdateSeqsUnlockSecondary(CCmdUI* pCmdUI) 
{		
	pCmdUI->Enable(Model_SecondaryAnimLockingActive(gTreeItemData.iModelHandle)
					&& 
					!Model_Sequence_IsLocked( gTreeItemData.iModelHandle, -1, false )
					&&
					!Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false)
					);
}



CString		strTreeItemTextToFind;
HTREEITEM	ghTreeItemFound;
HTREEITEM	ghTreeItemHeader;
HTREEITEM	ghTreeCurrentFind;	// if NZ, don't count finds until you've reached this (last find)

static void SearchTreeItem( HTREEITEM hTreeItem )
{
	// if doing a find-next, don't do anything in here until we've reached the prev find, then clear it
	if (ghTreeCurrentFind)
	{
		if (hTreeItem == ghTreeCurrentFind)
		{
			ghTreeCurrentFind = NULL;
		}
	}
	else
	{
		if (!ghTreeItemFound)	// so we find the first, not the last
		{
			CString strThisItem = gModViewTreeViewhandle->GetTreeCtrl().GetItemText(hTreeItem);
		
			strThisItem.MakeLower();
			if (strstr(strThisItem,strTreeItemTextToFind))
				ghTreeItemFound = hTreeItem;
		}
	}
}

void CModViewTreeView::OnFindNext()
{
	if (!strTreeItemTextToFind.IsEmpty() && ghTreeItemFound)
	{
		HTREEITEM hTreeItemPrevFound =	ghTreeItemFound;
					ghTreeCurrentFind=  ghTreeItemFound;
										ghTreeItemFound = NULL;	// so we can find again

		R_ApplyToTreeItem( ::SearchTreeItem, ghTreeItemHeader, false, false );

		if (!ghTreeItemFound)
			ghTreeItemFound = ghTreeItemHeader;	// restore search to header, so it can loop
		
		GetTreeCtrl().SelectSetFirstVisible(ghTreeItemFound);
		GetTreeCtrl().SelectItem(ghTreeItemFound);		
	}
}

void CModViewTreeView::OnSurfacesFind() 
{
	LPCSTR psSearch = GetString("Enter Surface name to search for...\n\n( Case insensitive, partial strings ok )");

	if (psSearch)
	{
		strTreeItemTextToFind	= psSearch;
		ghTreeItemFound			= NULL;
		ghTreeItemHeader		= ghTreeItem_RButtonMenu;
		ghTreeCurrentFind		= NULL;

		R_ApplyToTreeItem( ::SearchTreeItem, ghTreeItemHeader );

		if (ghTreeItemFound)
		{
			GetTreeCtrl().SelectSetFirstVisible(ghTreeItemFound);
			GetTreeCtrl().SelectItem(ghTreeItemFound);
		}
	}
}

void CModViewTreeView::OnBonesFind() 
{
	LPCSTR psSearch = GetString("Enter Bone name to search for...\n\n( Case insensitive, partial strings ok )");

	if (psSearch)
	{
		strTreeItemTextToFind	= psSearch;
		ghTreeItemFound			= NULL;
		ghTreeItemHeader		= ghTreeItem_RButtonMenu;
		ghTreeCurrentFind		= NULL;

		R_ApplyToTreeItem( ::SearchTreeItem, ghTreeItemHeader );

		if (ghTreeItemFound)
		{
			GetTreeCtrl().SelectSetFirstVisible(ghTreeItemFound);
			GetTreeCtrl().SelectItem(ghTreeItemFound);
		}
	}
}


// returns NULL if CANCEL, else input string
//
LPCSTR GetString(LPCSTR psPrompt, LPCSTR psDefault /*=NULL*/, bool bLowerCaseTheResult /*= true*/)
{
	static CString strReturn;

	CGetString Input(psPrompt,&strReturn,psDefault);
	if (Input.DoModal() == IDOK)
	{
		strReturn.TrimLeft();
		strReturn.TrimRight();

		if (bLowerCaseTheResult)
			strReturn.MakeLower();

		return (LPCSTR)strReturn;
	}

	return NULL;
}



void CModViewTreeView::OnModelFindany() 
{
	LPCSTR psSearch = GetString("Enter TreeItemText to search for...\n\n( Case insensitive, partial strings ok )");

	if (psSearch)
	{
		strTreeItemTextToFind	= psSearch;
		ghTreeItemFound			= NULL;
		ghTreeItemHeader		= ghTreeItem_RButtonMenu;
		ghTreeCurrentFind		= NULL;

		R_ApplyToTreeItem( ::SearchTreeItem, ghTreeItemHeader );

		if (ghTreeItemFound)
		{
			GetTreeCtrl().SelectSetFirstVisible(ghTreeItemFound);
			GetTreeCtrl().SelectItem(ghTreeItemFound);
		}
	}
}



static void ReEvalSequenceText( HTREEITEM hTreeItem )
{
	TreeItemData_t	TreeItemData;
					TreeItemData.uiData = gModViewTreeViewhandle->GetTreeCtrl().GetItemData(hTreeItem);

	if (TreeItemData.iItemType == TREEITEMTYPE_SEQUENCE)
	{
		LPCSTR psNewText = Model_Sequence_GetTreeName(TreeItemData.iModelHandle, TreeItemData.iItemNumber);

		gModViewTreeViewhandle->GetTreeCtrl().SetItemText( hTreeItem, psNewText );
	}
}

void CModViewTreeView::OnSequencesViewfullpath() 
{
	AppVars.bFullPathsInSequenceTreeitems = !AppVars.bFullPathsInSequenceTreeitems;

	R_ApplyToTreeItem( ::ReEvalSequenceText, GetTreeCtrl().GetRootItem()/* ghTreeItem_RButtonMenu */ );		
}

void CModViewTreeView::OnUpdateEthnicApplywithsurfaces(CCmdUI* pCmdUI) 
{
	HTREEITEM hTreeItemParent = GetTreeCtrl().GetParentItem(ghTreeItem_RButtonMenu);
	CString strParentSkin = GetTreeCtrl().GetItemText(hTreeItemParent);	

	pCmdUI->Enable( Model_SkinHasSurfacePrefs( gTreeItemData.iModelHandle, strParentSkin) );
}

void CModViewTreeView::OnEthnicApplywithsurfaces() 
{
	HTREEITEM hTreeItemParent = GetTreeCtrl().GetParentItem(ghTreeItem_RButtonMenu);
	CString strParentSkin = GetTreeCtrl().GetItemText(hTreeItemParent);	
	CString strThisEthnic = GetTreeCtrl().GetItemText(ghTreeItem_RButtonMenu);	
	
	Model_ApplyEthnicSkin( gTreeItemData.iModelHandle, strParentSkin, strThisEthnic, true, true );
}


void CModViewTreeView::OnEthnicApplywithsurfacedefaulting() 
{
	HTREEITEM hTreeItemParent = GetTreeCtrl().GetParentItem(ghTreeItem_RButtonMenu);
	CString strParentSkin = GetTreeCtrl().GetItemText(hTreeItemParent);	
	CString strThisEthnic = GetTreeCtrl().GetItemText(ghTreeItem_RButtonMenu);	
	
	Model_ApplyEthnicSkin( gTreeItemData.iModelHandle, strParentSkin, strThisEthnic, false, true );
}

void CModViewTreeView::OnSeqMultilock() 
{
	Model_MultiSeq_Add(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true);
}

void CModViewTreeView::OnUpdateSeqMultilock(CCmdUI* pCmdUI) 
{
	bool bActive		= Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true) ;
	int iNumSeqEntries	= Model_MultiSeq_GetNumEntries( gTreeItemData.iModelHandle, true);

	if ( bActive || iNumSeqEntries )
	{
		pCmdUI->SetText("Add to Multi-Lock sequences");
	}
	else
	{
		pCmdUI->SetText("Start Multi-Locking with this sequence");
	}

	pCmdUI->Enable( ( bActive || !iNumSeqEntries )
					&&
					!Model_MultiSeq_AlreadyContains(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true)
					);
}

void CModViewTreeView::OnUpdateSeqMultilockSecondary(CCmdUI* pCmdUI) 
{
	bool bActive		= Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false) ;
	int iNumSeqEntries	= Model_MultiSeq_GetNumEntries( gTreeItemData.iModelHandle, false);

	if ( bActive || iNumSeqEntries )
	{
		pCmdUI->SetText("Add to Secondary Multi-Lock sequences");
	}
	else
	{
		pCmdUI->SetText("Start Secondary Multi-Locking with this sequence");
	}
	pCmdUI->Enable( Model_SecondaryAnimLockingActive(gTreeItemData.iModelHandle) 
					&&
					(bActive || !iNumSeqEntries)
					&&
					!Model_MultiSeq_AlreadyContains(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false)
					);
}

void CModViewTreeView::OnSeqMultilockSecondary() 
{
	Model_MultiSeq_Add(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false);
}

// this is now a checkitem with toggle, but couldn't be bothered renaming all the MFC stuff...
//
void CModViewTreeView::OnMultiseqsUnlockPrimary() 
{
	Model_MultiSeq_SetActive(gTreeItemData.iModelHandle, true, !Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true));	
}

void CModViewTreeView::OnUpdateMultiseqsUnlockPrimary(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true) );
}

// this is now a checkitem with toggle, but couldn't be bothered renaming all the MFC stuff...
//
void CModViewTreeView::OnMultiseqsUnlockSecondary() 
{
	Model_MultiSeq_SetActive(gTreeItemData.iModelHandle, false, !Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false));
}

void CModViewTreeView::OnUpdateMultiseqsUnlockSecondary(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false) );
	pCmdUI->Enable  ( Model_SecondaryAnimLockingActive( gTreeItemData.iModelHandle ));
}

void CModViewTreeView::OnSeqsDeletelastPrimary() 
{
	Model_MultiSeq_DeleteLast( gTreeItemData.iModelHandle, true );
}

void CModViewTreeView::OnUpdateSeqsDeletelastPrimary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_MultiSeq_IsActive	( gTreeItemData.iModelHandle, true)
					&&
					Model_MultiSeq_GetNumEntries( gTreeItemData.iModelHandle, true )
					);
}

void CModViewTreeView::OnSeqsDeleteallPrimary() 
{
	Model_MultiSeq_Clear( gTreeItemData.iModelHandle, true );
}

void CModViewTreeView::OnUpdateSeqsDeleteallPrimary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_MultiSeq_IsActive	( gTreeItemData.iModelHandle, true)
					&&
					Model_MultiSeq_GetNumEntries( gTreeItemData.iModelHandle, true ) 
					);
}

void CModViewTreeView::OnSeqsDeletelastSecondary() 
{
	Model_MultiSeq_DeleteLast( gTreeItemData.iModelHandle, false );
}

void CModViewTreeView::OnUpdateSeqsDeletelastSecondary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_MultiSeq_IsActive	( gTreeItemData.iModelHandle, false)
					&&
					Model_MultiSeq_GetNumEntries( gTreeItemData.iModelHandle, false )
					);
}

void CModViewTreeView::OnSeqsDeleteallSecondary() 
{
	Model_MultiSeq_Clear( gTreeItemData.iModelHandle, false );
}

void CModViewTreeView::OnUpdateSeqsDeleteallSecondary(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_MultiSeq_IsActive	( gTreeItemData.iModelHandle, false)
					&&
					Model_MultiSeq_GetNumEntries( gTreeItemData.iModelHandle, false )
					);
}

void CModViewTreeView::OnSeqMultilockDelete() 
{
	Model_MultiSeq_Delete( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true);
}

void CModViewTreeView::OnUpdateSeqMultilockDelete(CCmdUI* pCmdUI) 
{	
	pCmdUI->Enable( Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true)
					&&
					Model_MultiSeq_AlreadyContains(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true)
					);
}

void CModViewTreeView::OnSeqMultilockSecondaryDelete() 
{
	Model_MultiSeq_Delete( gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false);
}

void CModViewTreeView::OnUpdateSeqMultilockSecondaryDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, false)
					&&
					Model_MultiSeq_AlreadyContains(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, false)
					);
}

void CModViewTreeView::OnGlmsurfaceSetasroot() 
{
	Model_SetG2SurfaceRootOverride(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber);
}

void CModViewTreeView::OnUpdateGlmsurfaceSetasroot(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_GetG2SurfaceRootOverride(gTreeItemData.iModelHandle) != gTreeItemData.iItemNumber );
}

void CModViewTreeView::OnTreeSurfacesClearroot() 
{
	Model_SetG2SurfaceRootOverride(gTreeItemData.iModelHandle, -1);	
}

void CModViewTreeView::OnUpdateTreeSurfacesClearroot(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_GetG2SurfaceRootOverride(gTreeItemData.iModelHandle) != -1);	
}



// convenience feature James wanted...
//
void CModViewTreeView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
//	OutputDebugString("OnLButtonDblClk\n");

	UINT nHitFlags = 0;
	HTREEITEM hTreeItem_Clicked = GetTreeCtrl().HitTest( point, &nHitFlags );
	
	TreeItemData_t	TreeItemData={0};
	if (!hTreeItem_Clicked)	// if we didn't physically click on something, see if there's anything selected
	{
		hTreeItem_Clicked = GetTreeCtrl().GetSelectedItem();
	}

	if (hTreeItem_Clicked)
	{
		TreeItemData.uiData = GetTreeCtrl().GetItemData(hTreeItem_Clicked);

		if (TreeItemData.iModelHandle)	// valid?  (should be)
		{
			gTreeItemData.uiData = TreeItemData.uiData;	// may as well copy to global version
			ghTreeItem_RButtonMenu = hTreeItem_Clicked;

			switch (TreeItemData.iItemType)
			{
				case TREEITEMTYPE_SEQUENCE:
				{
					// multiseqlock or single lock?...
					//
					if (Model_MultiSeq_IsActive(gTreeItemData.iModelHandle, true))
					{
						Model_MultiSeq_Add(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true);
					}
					else
					{
						Model_Sequence_Lock(gTreeItemData.iModelHandle, gTreeItemData.iItemNumber, true);
						ModelList_Rewind();
					}
				}
				break;

				case TREEITEMTYPE_OLDSKIN:
				{
					CString strSkin(GetTreeCtrl().GetItemText(ghTreeItem_RButtonMenu));
					Model_ApplyOldSkin( gTreeItemData.iModelHandle, strSkin );
				}
				break;
			}
		}
	}
	else
	{
		hTreeItem_Clicked = GetTreeCtrl().GetRootItem();
		TreeItemData.iItemType		= TREEITEMTYPE_MODELNAME;
		TreeItemData.iModelHandle	= Model_GetPrimaryHandle();	// default to primary if not clicked on something specific
	}

	CTreeView::OnLButtonDblClk(nFlags, point);
}

void CModViewTreeView::OnSequencesSortalphabetically() 
{
	AppVars.bSortSequencesByAlpha = !AppVars.bSortSequencesByAlpha;

	ModelTree_InsertSequences( gTreeItemData.iModelHandle, ghTreeItem_RButtonMenu );	
}

void CModViewTreeView::OnUpdateSequencesSortalphabetically(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AppVars.bSortSequencesByAlpha );	
}

void CModViewTreeView::OnGlmsurfaceClearroot() 
{
	Model_SetG2SurfaceRootOverride(gTreeItemData.iModelHandle, -1);	
}

void CModViewTreeView::OnUpdateGlmsurfaceClearroot(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( Model_GetG2SurfaceRootOverride(gTreeItemData.iModelHandle) == gTreeItemData.iItemNumber );
}

