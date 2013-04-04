#if !defined(AFX_MODVIEWTREEVIEW_H__EFFD4A64_9FB9_11D4_8A94_00500424438B__INCLUDED_)
#define AFX_MODVIEWTREEVIEW_H__EFFD4A64_9FB9_11D4_8A94_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ModViewTreeView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CModViewTreeView view

class CModViewTreeView : public CTreeView
{
protected:
	CModViewTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CModViewTreeView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModViewTreeView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CModViewTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;	
#endif
	void UpdateUI(CMenu* pMenu);

	// Generated message map functions
protected:
	//{{AFX_MSG(CModViewTreeView)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTreeModelExpandall();
	afx_msg void OnTreeModelContractall();
	afx_msg void OnGlmsurfaceInfo();
	afx_msg void OnTreeModelInfo();
	afx_msg void OnGlmsurfaceOff();
	afx_msg void OnUpdateGlmsurfaceOff(CCmdUI* pCmdUI);
	afx_msg void OnGlmsurfaceOn();
	afx_msg void OnUpdateGlmsurfaceOn(CCmdUI* pCmdUI);
	afx_msg void OnGlmsurfaceNodescendants();
	afx_msg void OnUpdateGlmsurfaceNodescendants(CCmdUI* pCmdUI);
	afx_msg void OnSeqLock();
	afx_msg void OnUpdateSeqLock(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSeqsUnlockall(CCmdUI* pCmdUI);
	afx_msg void OnSeqsUnlockall();
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeSurfacesExpandall();
	afx_msg void OnTreeBonesExpandall();
	afx_msg void OnGlmBonebolt();
	afx_msg void OnGlmBoneclearbolt();
	afx_msg void OnUpdateGlmBoneclearbolt(CCmdUI* pCmdUI);
	afx_msg void OnGlmboneInfo();
	afx_msg void OnTreeModelUnboltme();
	afx_msg void OnUpdateTreeModelUnboltme(CCmdUI* pCmdUI);
	afx_msg void OnUpdateJunk(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGlmboneTitle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGlmsurfaceTitle(CCmdUI* pCmdUI);
	afx_msg void OnSeqUnlock();
	afx_msg void OnUpdateSeqUnlock(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSeqTitle(CCmdUI* pCmdUI);
	afx_msg void OnGlmtagsurfaceInfo();
	afx_msg void OnGlmtagsurfaceBolt();
	afx_msg void OnGlmtagsurfaceDeletebolt();
	afx_msg void OnUpdateGlmtagsurfaceDeletebolt(CCmdUI* pCmdUI);
	afx_msg void OnTreeTagsurfacesExpandall();
	afx_msg void OnUpdateGlmtagsurfaceTitle(CCmdUI* pCmdUI);
	afx_msg void OnTreeSurfacesAlldefaultoffOn();
	afx_msg void OnUpdateTreeSurfacesAlldefaultoffOn(CCmdUI* pCmdUI);
	afx_msg void OnTreeSurfacesAlldefaultoffOff();
	afx_msg void OnUpdateTreeSurfacesAlldefaultoffOff(CCmdUI* pCmdUI);
	afx_msg void OnSurfaceBolt();
	afx_msg void OnUpdateSurfaceBolt(CCmdUI* pCmdUI);
	afx_msg void OnSurfaceDeletemodelboltedtothissurface();
	afx_msg void OnUpdateSurfaceDeletemodelboltedtothissurface(CCmdUI* pCmdUI);
	afx_msg void OnSkinsValidate();
	afx_msg void OnSkinValidate();
	afx_msg void OnExpandall();
	afx_msg void OnVariantApply();
	afx_msg void OnEthnicApply();
	afx_msg void OnExpandAll();
	afx_msg void OnSkinExpandall();
	afx_msg void OnOldskinsValidate();
	afx_msg void OnOldskinValidate();
	afx_msg void OnOldskinApply();
	afx_msg void OnGlmBoneLoweranimstart();
	afx_msg void OnUpdateGlmBoneLoweranimstart(CCmdUI* pCmdUI);
	afx_msg void OnBonesClearsecondaryanim();
	afx_msg void OnUpdateBonesClearsecondaryanim(CCmdUI* pCmdUI);
	afx_msg void OnSeqLockSecondary();
	afx_msg void OnUpdateSeqLockSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSeqUnlockSecondary();
	afx_msg void OnUpdateSeqUnlockSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSeqsUnlockPrimary();
	afx_msg void OnUpdateSeqsUnlockPrimary(CCmdUI* pCmdUI);
	afx_msg void OnSeqsUnlockSecondary();
	afx_msg void OnUpdateSeqsUnlockSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSurfacesFind();
	afx_msg void OnBonesFind();
	afx_msg void OnFindNext();
	afx_msg void OnModelFindany();
	afx_msg void OnSequencesViewfullpath();
	afx_msg void OnUpdateEthnicApplywithsurfaces(CCmdUI* pCmdUI);
	afx_msg void OnEthnicApplywithsurfaces();
	afx_msg void OnTreeSurfacesAlldefaultoffDefault();
	afx_msg void OnEthnicApplywithsurfacedefaulting();
	afx_msg void OnSeqMultilock();
	afx_msg void OnUpdateSeqMultilock(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSeqMultilockSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSeqMultilockSecondary();
	afx_msg void OnMultiseqsUnlockPrimary();
	afx_msg void OnUpdateMultiseqsUnlockPrimary(CCmdUI* pCmdUI);
	afx_msg void OnMultiseqsUnlockSecondary();
	afx_msg void OnUpdateMultiseqsUnlockSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSeqsDeletelastPrimary();
	afx_msg void OnUpdateSeqsDeletelastPrimary(CCmdUI* pCmdUI);
	afx_msg void OnSeqsDeleteallPrimary();
	afx_msg void OnUpdateSeqsDeleteallPrimary(CCmdUI* pCmdUI);
	afx_msg void OnSeqsDeletelastSecondary();
	afx_msg void OnUpdateSeqsDeletelastSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSeqsDeleteallSecondary();
	afx_msg void OnUpdateSeqsDeleteallSecondary(CCmdUI* pCmdUI);
	afx_msg void OnSeqMultilockDelete();
	afx_msg void OnUpdateSeqMultilockDelete(CCmdUI* pCmdUI);
	afx_msg void OnSeqMultilockSecondaryDelete();
	afx_msg void OnUpdateSeqMultilockSecondaryDelete(CCmdUI* pCmdUI);
	afx_msg void OnGlmsurfaceSetasroot();
	afx_msg void OnUpdateGlmsurfaceSetasroot(CCmdUI* pCmdUI);
	afx_msg void OnTreeSurfacesClearroot();
	afx_msg void OnUpdateTreeSurfacesClearroot(CCmdUI* pCmdUI);
	afx_msg void OnGlmAddbonebolt();
	afx_msg void OnSurfaceAddbolt();
	afx_msg void OnGlmtagsurfaceAddbolt();
	afx_msg void OnUpdateSurfaceAddbolt(CCmdUI* pCmdUI);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSequencesSortalphabetically();
	afx_msg void OnUpdateSequencesSortalphabetically(CCmdUI* pCmdUI);
	afx_msg void OnGlmsurfaceClearroot();
	afx_msg void OnUpdateGlmsurfaceClearroot(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	void R_ApplyToTreeItem( void (*pFunction) ( HTREEITEM hTreeItem ), HTREEITEM hTreeItem, bool bProcessSiblings = false, bool bSkipProcessingOfInitialItem = false );

	// new helper functions I added...
public:
	BOOL	  DeleteAllItems();
	HTREEITEM InsertItem(LPCTSTR psName, HTREEITEM hParent, UINT32 uiUserData = NULL, HTREEITEM hInsertAfter = TVI_LAST);	
	HTREEITEM GetRootItem();
};


extern CModViewTreeView* gModViewTreeViewhandle;
LPCSTR GetString(LPCSTR psPrompt, LPCSTR psDefault = NULL, bool bLowerCaseTheResult = true);
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODVIEWTREEVIEW_H__EFFD4A64_9FB9_11D4_8A94_00500424438B__INCLUDED_)
