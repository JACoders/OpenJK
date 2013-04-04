// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "bits.h"

#if !defined(AFX_MAINFRM_H__61504888_AF87_11D4_8A97_00500424438B__INCLUDED_)
#define AFX_MAINFRM_H__61504888_AF87_11D4_8A97_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void LoadModel(LPCSTR psFullPathedFileName);
	void BoltModel(ModelHandle_t hModel, LPCSTR psBoltName, LPCSTR psFullPathedName);

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	afx_msg void OnErrorboxOff();
	afx_msg void OnErrorboxOn();
	afx_msg void OnEditM4();
	afx_msg void OnStartanim();
	afx_msg void OnStartanimwrap();
	afx_msg void OnStopanim();
	afx_msg void OnLockSequences();
	afx_msg void OnUnlockallseqs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__61504888_AF87_11D4_8A97_00500424438B__INCLUDED_)
