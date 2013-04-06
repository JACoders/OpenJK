// Model.h


#pragma warning( disable : 4786 )  // identifier was truncated 

#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;


class CModel
{
public:
	CModel();
	virtual ~CModel();

	virtual void Delete();
	static CModel* Create(CComment* comments = NULL);

	void Write(CTxtFile* file);	
	bool WriteExternal(bool bPromptForNames, bool& bCFGWritten);
	bool HasGLA(void);
	LPCSTR GLAName(void);

	void SetNext(CModel* next);
	CModel* GetNext();

	bool DoProperties();

	void AddComment(CComment* comment);
	CComment* GetFirstComment();
	CComment* ExtractComments();

	bool ContainsFile(LPCSTR psFilename);
	int AnimEnumInUse(LPCSTR psAnimEnum);
	void AddSequence(CSequence* sequence);
	void DeleteSequence(CSequence* deleteSequence);
	CSequence* GetFirstSequence();
	void Resequence(bool bReScanASEFiles = false);
	int GetTotSequences();
	int GetTotMasterSequences();
	void ReOrderSequences();
	void GetMasterEnumBoundaryFrameNumbers(int *piFirstFrameAfterBOTH, int *piFirstFrameAfterTORSO);

	LPCTSTR GetName();
	void SetName(LPCTSTR name);
	void DeriveName(LPCTSTR fromname);
	void SetPath(LPCTSTR path);
	LPCTSTR GetPath();

	void SetOrigin(int x, int y, int z);
	void SetOriginX(int x);
	void SetOriginY(int y);
	void SetOriginZ(int z);
	int GetOriginX();
	int GetOriginY();
	int GetOriginZ();

	void SetParms(int skipStart, int skipEnd, int totFrames, int headFrames);
	void SetTotFrames(int value);
	int GetTotFrames();

	void SetUserSelectionBool(bool bSelected = true);
	bool GetUserSelectionBool();

	void SetConvertType(int iType);
	int	 GetConvertType(void);
	bool IsGhoul2(void);

	void	SetKeepMotion(bool bKeepMotion);
	bool	GetKeepMotion(void);
	void	SetSmooth(bool bSmooth);
	void	SetLoseDupVerts(bool bLoseDupVerts);
	void	SetMakeSkin(bool bMakeSkin);
	void	SetIgnoreBaseDeviations(bool bIgnore);
	void	SetSkew90(bool bSkew90);
	void	SetNoSkew90(bool bNoSkew90);
//	void	SetSkelPath(LPCSTR psPath);
	void	SetMakeSkelPath(LPCSTR psPath);
	bool	GetSmooth(void);
	bool	GetLoseDupVerts(void);
	bool	GetMakeSkin(void);
	bool	GetIgnoreBaseDeviations(void);
	bool	GetSkew90(void);
	bool	GetNoSkew90(void);
//	LPCSTR	GetSkelPath(void);
	LPCSTR	GetMakeSkelPath(void);
	void	SetScale(float fScale);
	float	GetScale(void);
	bool	GetPreQuat(void);
	void	SetPreQuat(bool bPreQuat);

	void	PCJList_Clear();
	void	PCJList_AddEntry(LPCSTR psEntry);
	int		PCJList_GetEntries();
	LPCSTR	PCJList_GetEntry(int iIndex);

protected:
	void Init(CComment* comments);

	CModel*				m_next;
	CComment*			m_comments;
	char*				m_name;
	char*				m_path;
	CSequence*			m_sequences;
	CSequence*			m_curSequence;

	int					m_totFrames;
	int					m_headFrames;
	int					m_originx;
	int					m_originy;
	int					m_originz;

	bool				m_bCurrentUserSelection;	

	int					m_iType;	// eg TK_AS_CONVERT, TK_AS_CONVERTMDX, TK_AS_CONVERTMDX_NOASK,
	bool				m_bSmooth;
	bool				m_bLoseDupVerts;
	bool				m_bKeepMotion;
	bool				m_bMakeSkin;
	float				m_fScale;
	bool				m_bIgnoreBaseDeviations;
	bool				m_bSkew90;
	bool				m_bNoSkew90;
	char*				m_psSkelPath;
	char*				m_psMakeSkelPath;
	vector <string>		m_vPCJList;
	bool				m_bPreQuat;
};
/////////////////////////////////////////////////////////////////////////////
// CModelPropPage dialog

class CModelPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CModelPropPage)

// Construction
public:
	CModelPropPage();
	~CModelPropPage();

	CModel*				m_model;
	bool*				m_soilFlag;

// Dialog Data
	//{{AFX_DATA(CModelPropPage)
	enum { IDD = IDD_PP_MODEL };
	BOOL	m_bSkew90;
	BOOL	m_bSmooth;
	CString	m_strSkelPath;
	int		m_iOriginX;
	int		m_iOriginY;
	int		m_iOriginZ;
	float	m_fScale;
	BOOL	m_bMakeSkin;
	BOOL	m_bLoseDupVerts;
	BOOL	m_bMakeSkel;
	CString	m_strNewPCJ;
	BOOL	m_bKeepMotion;
	BOOL	m_bPreQuat;
	//}}AFX_DATA

public:
	void AddPCJEntry(LPCSTR psPCJName);
	void DelPCJEntry(int iIndex);

	int GetPCJEntries(void);
	LPCSTR GetPCJEntry(int iIndex);


protected:
	vector <CString> m_PCJList;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CModelPropPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CModelPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckMakeskel();
	afx_msg void OnButtonDelpcj();
	afx_msg void OnButtonPcj();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void HandleItemGreying(void);	
	void PopulatePCJList(void);
};

extern bool gbReportMissingASEs;
extern int  giFixUpdatedASEFrameCounts;

