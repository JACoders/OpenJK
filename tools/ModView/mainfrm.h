// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__EFFD4A58_9FB9_11D4_8A94_00500424438B__INCLUDED_)
#define AFX_MAINFRM_H__EFFD4A58_9FB9_11D4_8A94_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

protected:
	CSplitterWnd m_splitter;


// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFileOpen();
	afx_msg void OnViewWireframe();
	afx_msg void OnUpdateViewWireframe(CCmdUI* pCmdUI);
	afx_msg void OnViewAlpha();
	afx_msg void OnUpdateViewAlpha(CCmdUI* pCmdUI);
	afx_msg void OnViewInterpolate();
	afx_msg void OnUpdateViewInterpolate(CCmdUI* pCmdUI);
	afx_msg void OnViewBilinear();
	afx_msg void OnUpdateViewBilinear(CCmdUI* pCmdUI);
	afx_msg void OnViewScreenshotFile();
	afx_msg void OnViewScreenshotClipboard();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnViewOrigin();
	afx_msg void OnUpdateViewOrigin(CCmdUI* pCmdUI);
	afx_msg void OnViewGlinfo();
	afx_msg void OnAnimationStart();
	afx_msg void OnAnimationStop();
	afx_msg void OnAnimationRewind();
	afx_msg void OnAnimationFaster();
	afx_msg void OnAnimationSlower();
	afx_msg void OnAnimationLerping();
	afx_msg void OnUpdateAnimationLerping(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnModelSaveAs();
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnFileWriteideal();
	afx_msg void OnAnimationNextframe();
	afx_msg void OnAnimationPrevframe();
	afx_msg void OnViewLod0();
	afx_msg void OnViewLod1();
	afx_msg void OnViewLod2();
	afx_msg void OnViewLod3();
	afx_msg void OnViewLod4();
	afx_msg void OnViewLod5();
	afx_msg void OnViewLod6();
	afx_msg void OnViewLod7();
	afx_msg void OnEditBgrndcolour();
	afx_msg void OnUpdateViewBonehilite(CCmdUI* pCmdUI);
	afx_msg void OnViewBonehilite();
	afx_msg void OnViewNormals();
	afx_msg void OnUpdateViewNormals(CCmdUI* pCmdUI);
	afx_msg void OnViewSurfacehilite();
	afx_msg void OnUpdateViewSurfacehilite(CCmdUI* pCmdUI);
	afx_msg void OnViewVertindexes();
	afx_msg void OnUpdateViewVertindexes(CCmdUI* pCmdUI);
	afx_msg void OnViewFovcycle();
	afx_msg void OnFileReadideal();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnFileRefreshtextures();
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePrintSetup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditTestfunction(CCmdUI* pCmdUI);
	afx_msg void OnEditTestfunction();
	afx_msg void OnFileResetviewparams();
	afx_msg void OnAnimationStartwithwrapforce();
	afx_msg void OnFileWritescript();
	afx_msg void OnFileReadscript();
	afx_msg void OnUpdateFileWritescript(CCmdUI* pCmdUI);
	afx_msg void OnViewSurfacehilitewithbonerefs();
	afx_msg void OnUpdateViewSurfacehilitewithbonerefs(CCmdUI* pCmdUI);
	afx_msg void OnViewTagsurfaces();
	afx_msg void OnUpdateViewTagsurfaces(CCmdUI* pCmdUI);
	afx_msg void OnViewTagsasrgb();
	afx_msg void OnUpdateViewTagsasrgb(CCmdUI* pCmdUI);
	afx_msg void OnPicmip0();
	afx_msg void OnUpdatePicmip0(CCmdUI* pCmdUI);
	afx_msg void OnPicmip1();
	afx_msg void OnUpdatePicmip1(CCmdUI* pCmdUI);
	afx_msg void OnPicmip2();
	afx_msg void OnUpdatePicmip2(CCmdUI* pCmdUI);
	afx_msg void OnPicmip3();
	afx_msg void OnUpdatePicmip3(CCmdUI* pCmdUI);
	afx_msg void OnPicmip4();
	afx_msg void OnPicmip5();
	afx_msg void OnPicmip6();
	afx_msg void OnPicmip7();
	afx_msg void OnViewRuler();
	afx_msg void OnUpdateViewRuler(CCmdUI* pCmdUI);
	afx_msg void OnViewForcewhite();
	afx_msg void OnUpdateViewForcewhite(CCmdUI* pCmdUI);
	afx_msg void OnViewScreenshotClean();
	afx_msg void OnUpdateViewScreenshotClean(CCmdUI* pCmdUI);
	afx_msg void OnViewVertweighting();
	afx_msg void OnUpdateViewVertweighting(CCmdUI* pCmdUI);
	afx_msg void OnViewBbox();
	afx_msg void OnUpdateViewBbox(CCmdUI* pCmdUI);
	afx_msg void OnViewFloor();
	afx_msg void OnUpdateViewFloor(CCmdUI* pCmdUI);
	afx_msg void OnEditSetfloorAbs();
	afx_msg void OnUpdateEditSetfloorAbs(CCmdUI* pCmdUI);
	afx_msg void OnEditSetfloorCurrent();
	afx_msg void OnUpdateEditSetfloorCurrent(CCmdUI* pCmdUI);
	afx_msg void OnViewBonefiltering();
	afx_msg void OnUpdateViewBonefiltering(CCmdUI* pCmdUI);
	afx_msg void OnEditSetboneweightThreshhold();
	afx_msg void OnUpdateEditSetboneweightThreshhold(CCmdUI* pCmdUI);
	afx_msg void OnEditBoneFilterINCThreshhold();
	afx_msg void OnEditBoneFilterDECThreshhold();
	afx_msg void OnViewCrackviewer();
	afx_msg void OnUpdateViewCrackviewer(CCmdUI* pCmdUI);
	afx_msg void OnViewUnshadowablesurfaces();
	afx_msg void OnUpdateViewUnshadowablesurfaces(CCmdUI* pCmdUI);
	afx_msg void OnFileViewSof2Npcs();
	afx_msg void OnUpdateFileViewSof2Npcs(CCmdUI* pCmdUI);
	afx_msg void OnEditAllowskeletonoverrides();
	afx_msg void OnUpdateEditAllowskeletonoverrides(CCmdUI* pCmdUI);
	afx_msg void OnViewDoublesidedpolys();
	afx_msg void OnUpdateViewDoublesidedpolys(CCmdUI* pCmdUI);
	afx_msg void OnEditTopmost();
	afx_msg void OnUpdateEditTopmost(CCmdUI* pCmdUI);
	afx_msg void OnViewTriindexes();
	afx_msg void OnUpdateViewTriindexes(CCmdUI* pCmdUI);
	afx_msg void OnFileViewJk2Bots();
	afx_msg void OnAnimationEndframe();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void StatusMessage(LPCTSTR message);
	afx_msg void OnFileBatchconvert();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__EFFD4A58_9FB9_11D4_8A94_00500424438B__INCLUDED_)
