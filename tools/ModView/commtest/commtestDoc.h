// commtestDoc.h : interface of the CCommtestDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMTESTDOC_H__6150488A_AF87_11D4_8A97_00500424438B__INCLUDED_)
#define AFX_COMMTESTDOC_H__6150488A_AF87_11D4_8A97_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CCommtestDoc : public CDocument
{
protected: // create from serialization only
	CCommtestDoc();
	DECLARE_DYNCREATE(CCommtestDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommtestDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCommtestDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CCommtestDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMTESTDOC_H__6150488A_AF87_11D4_8A97_00500424438B__INCLUDED_)
