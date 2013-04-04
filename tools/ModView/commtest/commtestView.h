// commtestView.h : interface of the CCommtestView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMTESTVIEW_H__6150488C_AF87_11D4_8A97_00500424438B__INCLUDED_)
#define AFX_COMMTESTVIEW_H__6150488C_AF87_11D4_8A97_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CCommtestView : public CTreeView
{
protected: // create from serialization only
	CCommtestView();
	DECLARE_DYNCREATE(CCommtestView)

// Attributes
public:
	CCommtestDoc* GetDocument();

	UINT	m_TimerHandle_Update100FPS;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommtestView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCommtestView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CCommtestView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in commtestView.cpp
inline CCommtestDoc* CCommtestView::GetDocument()
   { return (CCommtestDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern CCommtestView* gpCommTestView;

#endif // !defined(AFX_COMMTESTVIEW_H__6150488C_AF87_11D4_8A97_00500424438B__INCLUDED_)
