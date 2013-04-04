// ModView.h : main header file for the MODVIEW application
//

#if !defined(AFX_MODVIEW_H__EFFD4A54_9FB9_11D4_8A94_00500424438B__INCLUDED_)
#define AFX_MODVIEW_H__EFFD4A54_9FB9_11D4_8A94_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CModViewApp:
// See ModView.cpp for the implementation of this class
//

class CModViewApp : public CWinApp
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CModViewApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModViewApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CModViewApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODVIEW_H__EFFD4A54_9FB9_11D4_8A94_00500424438B__INCLUDED_)
