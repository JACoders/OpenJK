// ModViewView.h : interface of the CModViewView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODVIEWVIEW_H__EFFD4A5C_9FB9_11D4_8A94_00500424438B__INCLUDED_)
#define AFX_MODVIEWVIEW_H__EFFD4A5C_9FB9_11D4_8A94_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


enum eTimerHandles
{
	th_DO_NOT_USE = 0,	// system reserved for NULL timer
	th_100FPS = 1,
};


class CModViewView : public CView
{
protected: // create from serialization only
	CModViewView();
	DECLARE_DYNCREATE(CModViewView)

// Attributes
public:
	CModViewDoc* GetDocument();

// these must be cleared manually, currently in ::Create()
	HGLRC	m_hRC;	// rendering context
	HDC		m_hDC;	// device context
	int		m_iWindowWidth;
	int		m_iWindowDepth;
	UINT	m_TimerHandle_Update100FPS;


// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModViewView)
	public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CModViewView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CModViewView)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ModViewView.cpp
inline CModViewDoc* CModViewView::GetDocument()
   { return (CModViewDoc*)m_pDocument; }
#endif



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_MODVIEWVIEW_H__EFFD4A5C_9FB9_11D4_8A94_00500424438B__INCLUDED_)
