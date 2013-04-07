// Sequences.h

#define MAX_ADDITIONAL_SEQUENCES 4
#define iDEFAULTSEQFRAMESPEED 20

class CModel;

enum 
{
	TK_ASTERISK = TK_USERDEF,
	TK_ASE_FIRSTFRAME,
	TK_ASE_LASTFRAME,
	TK_ASE_FRAMESPEED,
};


// note that these are also in order of importance for the treeview sorting
//
typedef enum
{
	ET_BOTH = 0,
	ET_TORSO,
	ET_LEGS,
	ET_FACE,
	ET_INVALID,

} ENUMTYPE;

//

class CSequence
{
public:
	CSequence();
	virtual ~CSequence();

	virtual void Delete();
	static CSequence* _Create(bool bGenLoopFrame, bool bIsGLA, LPCTSTR path = NULL, int startFrame = 0, int targetFrame = 0, int frameCount = 0, int frameSpeed = iDEFAULTSEQFRAMESPEED, int frameSpeedFromHeader = iDEFAULTSEQFRAMESPEED, CComment* comments = NULL);
	static CSequence* CreateFromFile(LPCTSTR path, CComment* comments);

	void Write(CTxtFile* file, bool bPreQuat);
	void WriteExternal(CModel *pModel, CTxtFile* file, bool bMultiPlayerFormat);
	void ReadHeader();
	void ReadASEHeader(LPCSTR psFilename, int &iStartFrame, int &iFrameCount, int &iFrameSpeed, bool bCanSkipXSIRead = false);
	void ReadXSIHeader(LPCSTR psFilename, int &iStartFrame, int &iFrameCount, int &iFrameSpeed);
	bool Parse();

	CSequence* GetNext();
	void SetNext(CSequence* next);

	bool DoProperties();

	LPCTSTR GetPath();
	void SetPath(LPCTSTR path);

	bool ValidEnum();
	void SetValidEnum(bool value = true);

	int GetStartFrame();
	void SetStartFrame(int frame);
	int GetTargetFrame();
	void SetTargetFrame(int frame);
	int GetFrameCount();
	void SetFrameCount(int count);
	int GetFrameSpeed();
	void SetFrameSpeed(int speed);
	int GetFrameSpeedFromHeader();
	void SetFrameSpeedFromHeader(int speed);
	int GetLoopFrame();
	void SetLoopFrame(int loop);
	bool GetGenLoopFrame(void);
	void SetGenLoopFrame(bool bGenLoopFrame);

	void AddComment(CComment* comment);
	CComment* GetFirstComment();

	int GetDisplayIconForTree(CModel* pModel);
	LPCSTR GetDisplayNameForTree(CModel* pModel, bool bIncludeAnimEnum, bool bIncludeFrameDetails, bool bViewFrameDetails_Additional, CDC* pDC);
	LPCTSTR GetName();
	void SetName(LPCTSTR name);
	void DeriveName();

	int GetFill();
	void SetFill(int value);

	ENUMTYPE GetEnumTypeFromString(LPCSTR lpString);
	ENUMTYPE GetEnumType();
	LPCTSTR GetEnum();
	void SetEnum(LPCTSTR name);

	LPCTSTR GetSound();
	void SetSound(LPCTSTR name);

	LPCTSTR GetAction();
	void SetAction(LPCTSTR name);

	bool IsGLA();

	int						m_iSequenceNumber;	// temp-use during tree-sort/model-resequencing

	// this isn't a brilliant wy of doing things, but enables retro-fitting of extra data fairly painlessly...
	CSequence*	AdditionalSeqs[MAX_ADDITIONAL_SEQUENCES];
	int GetValidAdditionalSequences();
	bool AdditionalSequenceIsValid();

protected:
	void _Init(bool bGenLoopFrame, bool bIsGLA, LPCTSTR path, int startFrame, int targetFrame, int frameCount, int frameSpeed, int frameSpeedFromheader, CComment* comments);

	CSequence*				m_next;
	CComment*				m_comments;
	char*					m_path;
	char*					m_name;
	CString					m_enum;
	char*					m_sound;
	char*					m_action;
	int						m_fill;
	int						m_startFrame;
	int						m_targetFrame;
	int						m_frameCount;
	int						m_frameSpeed;
	int						m_iFrameSpeedFromHeader;
	int						m_loopFrame;
	bool					m_bIsGLA;
	bool					m_bGenLoopFrame;

	bool					m_validEnum;

//	static keywordArray_t	s_Symbols[];
//	static keywordArray_t	s_Keywords[];
};



/////////////////////////////////////////////////////////////////////////////
// CSequencePropPage dialog

class CSequencePropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSequencePropPage)

// Construction
public:
	CSequencePropPage();
	~CSequencePropPage();

	CSequence*				m_sequence;
	bool*					m_soilFlag;

// Dialog Data
	//{{AFX_DATA(CSequencePropPage)
	enum { IDD = IDD_PP_SEQUENCE };
	int		m_frameCount;
	int		m_frameSpeed;
	CString	m_path;
	int		m_startFrame;
	int		m_iLoopFrame;
	CString	m_AnimationEnum;
	CString	m_AnimationEnum2;
	CString	m_AnimationEnum3;
	CString	m_AnimationEnum4;
	CString	m_AnimationEnum5;
	int		m_frameCount2;
	int		m_frameCount3;
	int		m_frameCount4;
	int		m_frameCount5;
	int		m_frameSpeed2;
	int		m_frameSpeed3;
	int		m_frameSpeed4;
	int		m_frameSpeed5;
	int		m_iLoopFrame2;
	int		m_iLoopFrame3;
	int		m_iLoopFrame4;
	int		m_iLoopFrame5;
	int		m_startFrame2;
	int		m_startFrame3;
	int		m_startFrame4;
	int		m_startFrame5;
	BOOL	m_bGenLoopFrame;
	//}}AFX_DATA


	void OkOrApply();
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSequencePropPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void HandleAdditionalEditBoxesGraying();
	void HandleAllItemsGraying();
	// Generated message map functions
	//{{AFX_MSG(CSequencePropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonChooseanimationenum();
	afx_msg void OnButtonChooseanimationenum2();
	afx_msg void OnButtonChooseanimationenum3();
	afx_msg void OnButtonChooseanimationenum4();
	afx_msg void OnButtonChooseanimationenum5();
	afx_msg void OnButtonClearanimationenum();
	afx_msg void OnButtonClearanimationenum2();
	afx_msg void OnButtonClearanimationenum3();
	afx_msg void OnButtonClearanimationenum4();
	afx_msg void OnButtonClearanimationenum5();
	afx_msg void OnKillfocusStartframe();
	afx_msg void OnKillfocusStartframe2();
	afx_msg void OnKillfocusStartframe3();
	afx_msg void OnKillfocusStartframe4();
	afx_msg void OnKillfocusStartframe5();
	afx_msg void OnKillfocusLoopframe();
	afx_msg void OnKillfocusLoopframe2();
	afx_msg void OnKillfocusLoopframe3();
	afx_msg void OnKillfocusLoopframe4();
	afx_msg void OnKillfocusLoopframe5();
	afx_msg void OnKillfocusFramespeed5();
	afx_msg void OnKillfocusFramespeed4();
	afx_msg void OnKillfocusFramespeed3();
	afx_msg void OnKillfocusFramespeed2();
	afx_msg void OnKillfocusFramespeed();
	afx_msg void OnKillfocusFramecount5();
	afx_msg void OnKillfocusFramecount4();
	afx_msg void OnKillfocusFramecount3();
	afx_msg void OnKillfocusFramecount2();
	afx_msg void OnKillfocusFramecount();
	afx_msg void OnKillfocusEditAnimationenum5();
	afx_msg void OnKillfocusEditAnimationenum4();
	afx_msg void OnKillfocusEditAnimationenum3();
	afx_msg void OnKillfocusEditAnimationenum2();
	afx_msg void OnKillfocusEditAnimationenum();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

ENUMTYPE GetEnumTypeFromString(LPCSTR lpString);
bool IsEnumSeperator(LPCSTR lpString);
LPCSTR StripSeperatorStart(LPCSTR lpString);
void ReadASEHeader(LPCSTR psFilename, int &iStartFrame, int &iFrameCount, int &iFrameSpeed, bool bReadingGLA, bool bCanSkipXSIRead = false);

