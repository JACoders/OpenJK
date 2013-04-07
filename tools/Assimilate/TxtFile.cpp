// TxtFile.cpp

#include <afxwin.h> 
#include "TxtFile.h"

CTxtFile::CTxtFile()
{
}

CTxtFile::~CTxtFile()
{
}

CTxtFile* CTxtFile::Create(CFile* file)
{
	CTxtFile* newFile = new CTxtFile();
	newFile->Init(file);
	return newFile;
}

CTxtFile* CTxtFile::Create(LPCTSTR filename)
{
	CTxtFile* newFile = new CTxtFile();
	newFile->Init(filename);
	return newFile;
}

void CTxtFile::Delete()
{
	if (m_ownsFile && (m_file != NULL))
	{
		delete m_file;
		m_file = NULL;
	}
	delete this;
}

void CTxtFile::Init(LPCTSTR filename)
{
	try
	{
		m_file = new CFile(filename, CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive);
		m_ownsFile = true;
	}
	catch(CException* e)
	{
		m_file = NULL;
		e->Delete();
	}
}

void CTxtFile::Init(CFile* file)
{
	m_file = file;
	m_ownsFile = false;
}

void CTxtFile::Write(LPCTSTR string)
{
	ASSERT(string);
	m_file->Write(string, strlen(string));
}

void CTxtFile::Writeln(LPCTSTR string1, LPCTSTR string2, LPCTSTR string3)
{
	Write(string1);
	Write(string2);
	Writeln(string3);
}

void CTxtFile::Writeln(LPCTSTR string1, LPCTSTR string2)
{
	Write(string1);
	Writeln(string2);
}

void CTxtFile::Write(LPCTSTR string1, LPCTSTR string2, LPCTSTR string3)
{
	Write(string1);
	Write(string2);
	Write(string3);
}

void CTxtFile::Write(LPCTSTR string1, LPCTSTR string2)
{
	Write(string1);
	Write(string2);
}

void CTxtFile::Writeln(LPCTSTR string)
{
	Write(string);
	Writeln();
}

void CTxtFile::Writeln()
{
	Write("\r\n");
}

void CTxtFile::Write(int value)
{
	CString temp;
	temp.Format("%d", value);
	Write(temp);
}

void CTxtFile::Write(float value, int fractSize)
{
	CString temp;
	switch (fractSize)
	{
	case 1:
		temp.Format("%.1f", value);
		break;
	case 2:
		temp.Format("%.2f", value);
		break;
	case 3:
		temp.Format("%.3f", value);
		break;
	case 4:
		temp.Format("%.4f", value);
		break;
	case 5:
		temp.Format("%.5f", value);
		break;
	case 6:
		temp.Format("%.6f", value);
		break;
	default:
		temp.Format("%g", value);
		break;
	}
	Write(temp);
}

void CTxtFile::WriteString(LPCTSTR string)
{
	CString temp;
	temp.Format("\"%s\"", string);
	Write(temp);
}

// there is a slight problem with writing comments at the moment to do with spurious 0x0D bytes occasionally
//	being introduced. This may be due to designers sometimes using text editors that use CR-only line ends
//	rather than CR/LF pairs, and the tokenizer reader not stripping them. I could update the tokenizer source
//	member "GetStringValue()" but that's shadowed in a frightening number of programs, and I don't want to break
//	something miles away, so I'll just check for them here. Basically, you should never get 2 0x0D bytes next 
//	to each other, so each time I find one a pair I'll remove the first one.
//
void CTxtFile::WriteComment(LPCTSTR string, int indent)
{	
	const char sFind   [3]={0x0D,0x0D,0};	// a bit horrible really, but needs to be exact bytes
	const char sReplace[2]={0x0D,0};
	CString temp = string;

	while (temp.Replace(sFind,sReplace));	// after this we're down to 1 0x0D max

	int loc = temp.Find('\n');
	if (loc == -1)
	{
		// now we know there's no 0x0D/0x0A pairs then any remaining 0x0D bytes are orphans and should be disposed
		//	of (don't you just love it when text files are read in as binary?)
		//
		while (temp.Replace(sReplace,NULL));
		Space(indent);
		Write("//", temp);
		Writeln();
		return;
	}
	Space(indent);
	Writeln("/* ");
	while (loc > -1)
	{
		CString left = temp.Left(loc);
		temp = temp.Right(temp.GetLength() - loc - 1);
		Space(indent);
		Writeln(left);
		loc = temp.Find('\n');
	}
	Space(indent);
	Writeln(temp);
	Space(indent);
	Writeln("*/");
}

void CTxtFile::Space(int value)
{
	char* temp;
	temp = (char*)malloc(value + 1);
	ASSERT(temp);
	memset(temp, ' ', value);
	temp[value] = '\0';
	Write(temp);
	free(temp);
}
