#if !defined(AFX_BUILDALL_H__46894350_463F_4AC6_B84D_5378E3D50E7E__INCLUDED_)
#define AFX_BUILDALL_H__46894350_463F_4AC6_B84D_5378E3D50E7E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BuildAll.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBuildAll dialog

class CBuildAll : public CDialog
{
// Construction
public:
	CBuildAll(CString strPath, bool bPreValidate,
				CWnd* pParent = NULL);   // standard constructor

	void GetData(CString& strPath, bool& bPreValidate);

// Dialog Data
	//{{AFX_DATA(CBuildAll)
	enum { IDD = IDD_BUILD_ALL };
	BOOL	m_bPreValidateCARs;
	CString	m_strBuildPath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBuildAll)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBuildAll)
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BUILDALL_H__46894350_463F_4AC6_B84D_5378E3D50E7E__INCLUDED_)
