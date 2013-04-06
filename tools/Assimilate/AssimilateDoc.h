// AssimilateDoc.h : interface of the CAssimilateDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASSIMILATEDOC_H__2CCA554A_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_)
#define AFX_ASSIMILATEDOC_H__2CCA554A_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum 
{
	TK_AS_GRABINIT = TK_USERDEF,
	TK_AS_GRAB,
	TK_AS_GRAB_GLA,
	TK_AS_FILL,
	TK_AS_ACTION,
	TK_AS_SOUND,
	TK_AS_ENUM,
	TK_AS_GRABFINALIZE,
	TK_AS_CONVERT,
	TK_AS_CONVERTMDX,
	TK_AS_CONVERTMDX_NOASK,
	TK_AS_FRAMES,
	TK_AS_PLAYERPARMS,
	TK_AS_SCALE,
	TK_AS_PCJ,
	TK_AS_KEEPMOTION,
	TK_AS_ORIGIN,
	TK_AS_SMOOTH,
	TK_AS_LOSEDUPVERTS,
	TK_AS_MAKESKIN,
	TK_AS_IGNOREBASEDEVIATIONS,
	TK_AS_SKEW90,
	TK_AS_NOSKEW90,
	TK_AS_SKEL,
	TK_AS_MAKESKEL,
	TK_AS_LOOP,
	TK_AS_ADDITIONAL,
	TK_AS_PREQUAT,
	TK_AS_FRAMESPEED,
	TK_AS_QDSKIPSTART,	// useful so qdata can quickly skip extra stuff without having to know the syntax of what to skip
	TK_AS_QDSKIPSTOP,
	TK_AS_GENLOOPFRAME,
	TK_DOLLAR,
	TK_UNDERSCORE,
	TK_DASH,
	TK_DOT,
	TK_SLASH,
	TK_BACKSLASH,
};

enum
{
	AS_NOTHING,
	AS_NEWFILE,			
	AS_DELETECONTENTS,
	AS_FILESUPDATED,		
};

enum
{
	TABLE_ANY,
	TABLE_QDT,
	TABLE_GRAB,
	TABLE_CONVERT,
};

class CAssimilateDoc : public CDocument
{
protected: // create from serialization only
	CAssimilateDoc();
	DECLARE_DYNCREATE(CAssimilateDoc)

// Attributes
public:
	static LPCTSTR GetKeyword(int token, int table = 0);

// Operations
public:

	int GetNumModels();
	CModel*	GetFirstModel();
	void Resequence();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssimilateDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual BOOL DoFileSave();
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument_Actual(LPCTSTR lpszPathName, bool bCheckOut) ;
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAssimilateDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void ClearModelUserSelectionBools();
	CModel *GetCurrentUserSelectedModel();
	void DeleteModel(CModel *deleteModel);

protected:
	void Init();

	void Parse(CFile* file);
	void Parse(LPCSTR psFilename);
	void Write(CFile* file);

	void AddFile(LPCTSTR name);
	CModel* AddModel();	
	void EndModel();
	void ParseGrab(CTokenizer* tokenizer, int iGrabType);
	void ParseConvert(CTokenizer* tokenizer, int iTokenType);
	void AddComment(LPCTSTR comment);
	bool Validate(bool bInfoBoxAllowed = false, int iLODLevel = 0);
	bool Build(bool bAllowedToShowSuccessBox, int iLODLevel, bool bSkipSave);
	bool WriteCFGFiles(bool bPromptForNames, bool &bCFGWritten);

	CComment*				m_comments;
	CModel*					m_modelList;
	CModel*					m_curModel;	// fixme: update this in either usage or label to say only Mike's code uses it during loading, not elsewhere
	CModel*					m_lastModel;

	static keywordArray_t	s_Symbols[];
	static keywordArray_t	s_Keywords[];
	static keywordArray_t	s_grabKeywords[];
	static keywordArray_t	s_convertKeywords[];

// Generated message map functions
protected:
	//{{AFX_MSG(CAssimilateDoc)
	afx_msg void OnAddfiles();
	afx_msg void OnExternal();
	afx_msg void OnResequence();
	afx_msg void OnBuild();
	afx_msg void OnBuildMultiLOD();
	afx_msg void OnValidate();	
	afx_msg void OnCarWash();
	afx_msg void OnCarWashActual();
	afx_msg void OnValidateMultiLOD();
	afx_msg void OnViewAnimEnums();
	afx_msg void OnUpdateViewAnimEnums(CCmdUI* pCmdUI);
	afx_msg void OnViewFrameDetails();
	afx_msg void OnUpdateViewFrameDetails(CCmdUI* pCmdUI);
	afx_msg void OnUpdateResequence(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExternal(CCmdUI* pCmdUI);
	afx_msg void OnUpdateValidate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBuild(CCmdUI* pCmdUI);
	afx_msg void OnEditBuildall();
	afx_msg void OnEditBuildDependant();
	afx_msg void OnViewFramedetailsonadditionalsequences();
	afx_msg void OnUpdateViewFramedetailsonadditionalsequences(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditBuilddependant(CCmdUI* pCmdUI);
	afx_msg void OnEditLaunchmodviewoncurrent();
	afx_msg void OnUpdateEditLaunchmodviewoncurrent(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern bool gbCarWash_YesToXSIScan;
extern bool gbCarWash_DoingScan;
extern CString strCarWashErrors;
bool SendToNotePad(LPCSTR psWhatever, LPCSTR psLocalFileName);


#endif // !defined(AFX_ASSIMILATEDOC_H__2CCA554A_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_)
