// commtestDoc.cpp : implementation of the CCommtestDoc class
//

#include "stdafx.h"
#include "commtest.h"

#include "commtestDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommtestDoc

IMPLEMENT_DYNCREATE(CCommtestDoc, CDocument)

BEGIN_MESSAGE_MAP(CCommtestDoc, CDocument)
	//{{AFX_MSG_MAP(CCommtestDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommtestDoc construction/destruction

CCommtestDoc::CCommtestDoc()
{
	// TODO: add one-time construction code here

}

CCommtestDoc::~CCommtestDoc()
{
}

BOOL CCommtestDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CCommtestDoc serialization

void CCommtestDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCommtestDoc diagnostics

#ifdef _DEBUG
void CCommtestDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCommtestDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCommtestDoc commands
