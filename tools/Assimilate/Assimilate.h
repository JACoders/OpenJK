// Assimilate.h : main header file for the ASSIMILATE application
//

#if !defined(AFX_ASSIMILATE_H__2CCA5544_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_)
#define AFX_ASSIMILATE_H__2CCA5544_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

// not incredibly important, just used for guessing string lengths for display/output neatness...
//
#define APPROX_LONGEST_ASE_NAME 20
extern bool gbViewAnimEnums;
extern bool gbViewFrameDetails;
extern bool gbViewFrameDetails_Additional;

/////////////////////////////////////////////////////////////////////////////
// CAssimilateApp:
// See Assimilate.cpp for the implementation of this class
//
enum
{
	TK_ENUM = TK_USERDEF,
	TK_COMMA,
	TK_OBRACE,
	TK_CBRACE,
	TK_SEMICOLON,
};

// 1st CString is enum key, (eg) "BOTH_PAIN1", 2nd is comment (body text only, no comment chars)
//
typedef list< pair<CString,CString> > EnumTable_t;

class CAssimilateApp : public CWinApp
{
public:	
	CAssimilateApp();

	LPCSTR GetEnumComment(LPCSTR psEnum);
	bool ValidEnum(LPCTSTR name);

	bool GetMultiPlayerMode();
	LPCTSTR GetEnumFilename();
	LPCTSTR GetQDataFilename();
	DWORD GetBufferSize();
	LPCTSTR GetQuakeDir();

	bool SetMultiPlayerMode(bool bMultiPlayerMode);
	bool SetEnumFilename(LPCTSTR filename);
	bool SetQDataFilename(LPCTSTR filename);
	bool SetBufferSize(DWORD buffersize);
	bool SetQuakeDir(LPCTSTR psQuakeDir);	

	int GetEnumTableEntries();
	LPCSTR GetEnumEntry(int iIndex);

protected:
	void LoadEnumTable(LPCTSTR filename);
	void LoadRegistry();
	void SaveRegistry();

	EnumTable_t				m_enumTable;	
	static keywordArray_t	s_Symbols[];
	static keywordArray_t	s_Keywords[];

	bool					m_bMultiPlayerMode;
	CString					m_enumFilename;
	DWORD					m_buffersize;
	CString					m_QDataFilename;
	CString					m_QuakeDir;

	static const TCHAR c_prefSection[];
	static const TCHAR c_enumFilename[];
	static const TCHAR c_buffersize[];
	static const TCHAR c_QDataFilename[];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssimilateApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CAssimilateApp)
	afx_msg void OnAppAbout();
	afx_msg void OnProperties();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CAssimilatePropPage dialog

class CAssimilatePropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAssimilatePropPage)

// Construction
public:
	CAssimilatePropPage();
	~CAssimilatePropPage();

	bool*			m_soilFlag;

// Dialog Data
	//{{AFX_DATA(CAssimilatePropPage)
	enum { IDD = IDD_PP_PROPERTIES };
	DWORD	m_buffsize;
	CString	m_enumfilename;
	CString	m_qdata;
	CString	m_csQuakeDir;
	BOOL	m_bMultiPlayer;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAssimilatePropPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAssimilatePropPage)
	afx_msg void OnEnumBrowse();
	afx_msg void OnQdataBrowse();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDefaults();
	afx_msg void OnButtonDefaultsMulti();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ASSIMILATE_H__2CCA5544_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_)
