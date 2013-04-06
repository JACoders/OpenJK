// Comment.h

class CComment
{
public:
	CComment();
	virtual ~CComment();

	virtual void Delete();
	static CComment* Create(LPCTSTR comment = NULL);

	void Write(CTxtFile* file);

	void SetComment(LPCTSTR comment);
	LPCTSTR GetComment();

	void SetNext(CComment* next);
	CComment* GetNext();

protected:
	void Init(LPCTSTR comment);

	char*			m_comment;
	CComment*		m_next;
};

