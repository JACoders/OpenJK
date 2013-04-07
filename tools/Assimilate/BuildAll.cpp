// BuildAll.cpp : implementation file
//

#include "stdafx.h"
#include "Includes.h"
//#include "assimilate.h"
#include "BuildAll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBuildAll dialog


CBuildAll::CBuildAll(CString strPath, bool bPreValidate,
					CWnd* pParent /*=NULL*/)
	: CDialog(CBuildAll::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBuildAll)
	m_bPreValidateCARs = bPreValidate;
	m_strBuildPath = strPath;
	//}}AFX_DATA_INIT
}


void CBuildAll::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBuildAll)
	DDX_Check(pDX, IDC_CHECK_PREVALIDATECARS, m_bPreValidateCARs);
	DDX_Text(pDX, IDC_EDIT_BUILDPATH, m_strBuildPath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBuildAll, CDialog)
	//{{AFX_MSG_MAP(CBuildAll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBuildAll message handlers

void CBuildAll::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CBuildAll::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CBuildAll::GetData(CString& strPath, bool& bPreValidate)
{
	strPath		= m_strBuildPath;
	bPreValidate= !!m_bPreValidateCARs;
}

