// ModViewDoc.h : interface of the CModViewDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODVIEWDOC_H__EFFD4A5A_9FB9_11D4_8A94_00500424438B__INCLUDED_)
#define AFX_MODVIEWDOC_H__EFFD4A5A_9FB9_11D4_8A94_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CModViewDoc : public CDocument
{
protected: // create from serialization only
	CModViewDoc();
	DECLARE_DYNCREATE(CModViewDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModViewDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CModViewDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CModViewDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODVIEWDOC_H__EFFD4A5A_9FB9_11D4_8A94_00500424438B__INCLUDED_)
