// TxtFile.h

class CTxtFile
{
public:
	CTxtFile();
	~CTxtFile();
	static CTxtFile* Create(CFile* file);
	static CTxtFile* Create(LPCTSTR filename);
	void Delete();

	void WriteComment(LPCTSTR string, int indent = 0);
	void Write(LPCTSTR string);
	void Write(LPCTSTR string1, LPCTSTR string2);
	void Write(LPCTSTR string1, LPCTSTR string2, LPCTSTR string3);
	void Writeln();
	void Writeln(LPCTSTR string);
	void Writeln(LPCTSTR string1, LPCTSTR string2);
	void Writeln(LPCTSTR string1, LPCTSTR string2, LPCTSTR string3);
	void Write(int value);
	void Write(float value, int fractSize = -1);
	void WriteString(LPCTSTR string);
	void Space(int value = 1);
	CFile* GetFile() {return m_file;};
	int  IsValid(void) {return !!m_file;};

private:
	void Init(CFile* file);
	void Init(LPCTSTR filename);
	
	CFile*			m_file;
	bool			m_ownsFile;
};