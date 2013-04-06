// Tokenizer.cpp
#ifndef NOT_USING_MODULES
// !!! if you are not using modules, read BELOW !!!
#include "Module.h" // if you are not using modules, 
					// create an empty Module.h in your
					// project -- use of modules allows
					// the error handler to be overridden
					// with a custom CErrHandler
#endif
#include "Tokenizer.h"


enum
{
	DIR_INCLUDE = TK_USERDEF,
	DIR_IFDEF,
	DIR_IFNDEF,
	DIR_ENDIF,
	DIR_ELSE,
	DIR_DEFINE,
	DIR_UNDEFINE,
};

keywordArray_t CTokenizer::directiveKeywords[] =
{
	"include",		DIR_INCLUDE,
	"ifdef",		DIR_IFDEF,
	"ifndef",		DIR_IFNDEF,
	"endif",		DIR_ENDIF,
	"else",			DIR_ELSE,
	"define",		DIR_DEFINE,
	"undefine",		DIR_UNDEFINE,
	"",				TK_EOF,
};

keywordArray_t CTokenizer::errorMessages[] =
{
	"No Error",							TKERR_NONE,
	"Unknown Error",					TKERR_UNKNOWN,
	"Buffer creation failed",			TKERR_BUFFERCREATE,
	"Unrecognized symbol",				TKERR_UNRECOGNIZEDSYMBOL,
	"Duplicate symbol",					TKERR_DUPLICATESYMBOL,
	"String length exceeded",			TKERR_STRINGLENGTHEXCEEDED,
	"Identifier length exceeded",		TKERR_IDENTIFIERLENGTHEXCEEDED,
	"Expected integer",					TKERR_EXPECTED_INTEGER,
	"Expected identifier",				TKERR_EXPECTED_IDENTIFIER,
	"Expected string",					TKERR_EXPECTED_STRING,
	"Expected char",					TKERR_EXPECTED_CHAR,
	"Expected float",					TKERR_EXPECTED_FLOAT,
	"Unexpected token",					TKERR_UNEXPECTED_TOKEN,
	"Invalid directive",				TKERR_INVALID_DIRECTIVE,
	"Include file not found",			TKERR_INCLUDE_FILE_NOTFOUND,
	"Unmatched directive",				TKERR_UNMATCHED_DIRECTIVE,
	"Unexpected End of File",			TKERR_UNEXPECTED_EOF,
	"",									TKERR_USERERROR,
};

//
// CSymbol
//

CSymbol::CSymbol()
{
}

CSymbol::~CSymbol()
{
}

CSymbol* CSymbol::Create(LPCTSTR symbolName)
{
	CSymbol* retval = new CSymbol();
	retval->Init(symbolName);
	return retval;
}

LPCTSTR CSymbol::GetName()
{
	if (m_symbolName == NULL)
	{
		return "";
	}
	return m_symbolName;
}

void CSymbol::Init(LPCTSTR symbolName)
{
	if ((symbolName == NULL) || (strlen(symbolName) < 1))
	{
		m_symbolName = NULL;
		return;
	}
	m_symbolName = (char*)malloc(strlen(symbolName) + 1);
//	ASSERT(m_symbolName);
	strcpy(m_symbolName, symbolName);
}

void CSymbol::Delete()
{
	if (m_symbolName != NULL)
	{
		free(m_symbolName);
		m_symbolName = NULL;
	}
	delete this;
}

//
// CDirectiveSymbol
//

CDirectiveSymbol::CDirectiveSymbol()
{
}

CDirectiveSymbol::~CDirectiveSymbol()
{
}

CDirectiveSymbol* CDirectiveSymbol::Create(LPCTSTR symbolName)
{
	CDirectiveSymbol* retval = new CDirectiveSymbol();
	retval->Init(symbolName);
	return retval;
}

void CDirectiveSymbol::Init(LPCTSTR symbolName)
{
	CSymbol::Init(symbolName);
	m_value = NULL;
}

void CDirectiveSymbol::Delete()
{
	if (m_value != NULL)
	{
		free(m_value);
		m_value = NULL;
	}
	CSymbol::Delete();
}

void CDirectiveSymbol::SetValue(LPCTSTR value)
{
	if (m_value != NULL)
	{
		free(m_value);
		m_value = NULL;
	}
	if ((value == NULL) || (strlen(value) < 1))
	{
		return;
	}
	m_value = (char*)malloc(strlen(value) + 1);
	strcpy(m_value, value);
}

LPCTSTR CDirectiveSymbol::GetValue()
{
	return m_value;
}

//
// CIntSymbol
//

CIntSymbol::CIntSymbol()
{
}

CIntSymbol* CIntSymbol::Create(LPCTSTR symbolName, int value)
{
	CIntSymbol* retval = new CIntSymbol();
	retval->Init(symbolName, value);
	return retval;
}

void CIntSymbol::Delete()
{
	CSymbol::Delete();
}

void CIntSymbol::Init(LPCTSTR symbolName, int value)
{
	CSymbol::Init(symbolName);
	m_value = value;
}

int CIntSymbol::GetValue()
{
	return m_value;
}

//
// CSymbolTable
//

CSymbolTable::CSymbolTable()
{
	Init();
}

CSymbolTable::~CSymbolTable()
{
}

CSymbolTable* CSymbolTable::Create()
{
	CSymbolTable* retval = new CSymbolTable();
	retval->Init();
	return retval;
}

void CSymbolTable::Init()
{
}

void CSymbolTable::DiscardSymbols()
{
	for (symbolmap_t::iterator isymbol = m_symbols.begin(); isymbol != m_symbols.end(); isymbol++)
	{
		(*isymbol).second->Delete();
	}
	m_symbols.erase(m_symbols.begin(), m_symbols.end());
}

void CSymbolTable::Delete()
{
	DiscardSymbols();
	delete this;
}

void CSymbolTable::Iterate(void function(CSymbol*, DWORD, DWORD), DWORD parm1, DWORD parm2)
{
	for (symbolmap_t::iterator isymbol = m_symbols.begin(); isymbol != m_symbols.end(); isymbol++)
	{
		function((*isymbol).second, parm1, parm2);
	}
}

int CSymbolTable::GetNumEntries()
{
	return m_symbols.size();
}

LPCSTR CSymbolTable::GetEntry(int iIndex)
{
	if (iIndex<GetNumEntries())
	{
		int i=0;
		for (symbolmap_t::iterator isymbol = m_symbols.begin(); isymbol != m_symbols.end(); isymbol++)
		{
			if (i++==iIndex)
				return (*isymbol).first;
		}
	}
	return NULL;
}

bool CSymbolTable::AddSymbol(CSymbol* theSymbol)
{
	LPCTSTR name = theSymbol->GetName();
	
	symbolmap_t::iterator iter = m_symbols.find(name);
	if (iter != m_symbols.end())
	{
		return false;
	}
	m_symbols.insert(symbolmap_t::value_type(name, theSymbol));
	return true;
}

CSymbol* CSymbolTable::FindSymbol(LPCTSTR symbolName)
{
	if (symbolName)
	{
		symbolmap_t::iterator iter = m_symbols.find(symbolName);
		if (iter != m_symbols.end())
		{
			return (*iter).second;
		}
	}
	return NULL;
}

CSymbol* CSymbolTable::ExtractSymbol(LPCTSTR symbolName)
{
	symbolmap_t::iterator iter = m_symbols.find(symbolName);
	if (iter != m_symbols.end())
	{
		CSymbol* retval = (*iter).second;
		m_symbols.erase(iter);
		return retval;
	}
	return NULL;
}

void CSymbolTable::RemoveSymbol(LPCTSTR symbolName)
{
	m_symbols.erase(symbolName);
}

//
// CParseStream
//

CParseStream::CParseStream()
{
}

CParseStream::~CParseStream()
{
}

CParseStream* CParseStream::Create()
{
	return NULL;
}

void CParseStream::Delete()
{
	delete this;
}

bool CParseStream::Init()
{
	m_next = NULL;
	m_original = false;
	return true;
}

bool CParseStream::NextChar(byte& theByte)
{
	return false;
}

long CParseStream::GetRemainingSize()
{
	return 0;
}

bool CParseStream::CanEndOfFile()
{
	return false;
}

CParseStream* CParseStream::GetNext()
{
	return m_next;
}

void CParseStream::SetNext(CParseStream* next)
{
	m_next = next;
}

int CParseStream::GetCurLine()
{
	return 0;
}

void CParseStream::GetCurFilename(char** theBuff)
{
	*theBuff = NULL;
}

bool CParseStream::IsThisDefinition(void* theDefinition)
{
	return false;
}

//
// CParsePutBack
//

CParsePutBack::CParsePutBack()
{
}

CParsePutBack::~CParsePutBack()
{
}

CParsePutBack* CParsePutBack::Create(byte theByte, CParseStream* parent)
{
	CParsePutBack* curParsePutBack = new CParsePutBack();
	curParsePutBack->Init(theByte, parent);
	return curParsePutBack;
}

void CParsePutBack::Delete()
{
	delete this;
}

bool CParsePutBack::NextChar(byte& theByte)
{
	if (m_consumed)
	{
		return false;
	}
	theByte = m_byte;
	m_consumed = true;
	return true;
}

void CParsePutBack::Init(byte theByte, CParseStream* parent)
{
	CParseStream::Init();
	m_consumed = false;
	m_byte = theByte;
	m_curLine = 0;
	m_parent = parent;
	if (m_parent != NULL)
	{
		m_curLine = m_parent->GetCurLine();
	}
}

long CParsePutBack::GetRemainingSize()
{
	if (m_consumed)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int CParsePutBack::GetCurLine()
{
	return m_curLine;
}

void CParsePutBack::GetCurFilename(char** theBuff)
{
	if (m_parent == NULL)
	{
		*theBuff = NULL;
		return;
	}
	m_parent->GetCurFilename(theBuff);
}

//
// CParseFile
//

CParseFile::CParseFile()
{
}

CParseFile::~CParseFile()
{
}

CParseFile* CParseFile::Create()
{
	CParseFile* theParseFile = new CParseFile();
	
	if ( !theParseFile->Init() )
	{
		delete theParseFile;
		return NULL;
	}

	return theParseFile;
}

CParseFile* CParseFile::Create(LPCTSTR filename, CTokenizer* tokenizer, DWORD buffsize)
{
	CParseFile* theParseFile = new CParseFile();
	
	if ( theParseFile->Init(filename, tokenizer, buffsize) )
		return theParseFile;
	
	theParseFile->Delete();
	return NULL;
}

void CParseFile::Delete()
{
	if (m_buff != NULL)
	{
		free(m_buff);
		m_buff = NULL;
	}
	if (m_ownsFile && (m_fileHandle != NULL))
	{
		CloseHandle(m_fileHandle);
		m_fileHandle = NULL;
	}
	if (m_fileName != NULL)
	{
		free(m_fileName);
		m_fileName = NULL;
	}
	delete this;
}

bool CParseFile::Init()
{
	m_fileHandle = NULL;
	m_buff = NULL;
	m_ownsFile = false;
	m_curByte = NULL;
	m_curLine = 1;
	m_fileName = NULL;
	m_usesBuffer = false;
	m_buffsize = 0;
	m_buffByte = 0;
	bool retval = CParseStream::Init();
	m_original = true;
	return retval;
}

DWORD CParseFile::GetFileSize()
{
	DWORD dwCur = SetFilePointer(m_fileHandle, 0L, NULL, FILE_CURRENT);
	DWORD dwLen = SetFilePointer(m_fileHandle, 0, NULL, FILE_END);
	SetFilePointer(m_fileHandle, dwCur, NULL, FILE_BEGIN);
	return dwLen;
}

bool CParseFile::CanEndOfFile()
{
	return true;
}

void CParseFile::Read(void* buff, UINT buffsize)
{
	DWORD bytesRead;
	ReadFile(m_fileHandle, buff, buffsize, &bytesRead, NULL);
}

bool CParseFile::Init(LPCTSTR filename, CTokenizer* tokenizer, DWORD buffsize)
{
	CParseStream::Init();
	m_fileName = (char*)malloc(strlen(filename) + 1);
	strcpy(m_fileName, filename);
		
	DWORD dwAccess = GENERIC_READ;
	DWORD dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = 0;
	DWORD dwCreateFlag = OPEN_EXISTING;

	//need to get the game path to complete filename
	m_fileHandle = CreateFile(filename, dwAccess, dwShareMode, &sa, dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_fileHandle == (HANDLE)-1)
	{		
		tokenizer->Error(TKERR_INCLUDE_FILE_NOTFOUND, filename);
		//
		// if you don't do this, you get a memory leak when the Init() code below orphans this handle 
		//	(malloc'd at function top), I know it'll always be NZ, but I just prefer handling memptrs legally...-slc
		//
		if (m_fileName)
		{
			free(m_fileName);	
			m_fileName = NULL;	// I know this gets zeroed in Init(), but I really hate leaving bad ptrs!
		}
		Init();

		return false;			
	}

	m_filesize = GetFileSize();
	if (buffsize > 0)
	{
		m_usesBuffer = true;
		if (buffsize > m_filesize)
		{
			buffsize = m_filesize;
		}
	}
	else
	{
		buffsize = m_filesize;
	}
	m_buff = (byte*)malloc(buffsize);
	if (m_buff == NULL)
	{
		tokenizer->Error(TKERR_BUFFERCREATE);
		Init();
		return false;
	}
	Read(m_buff, buffsize);
	m_buffsize = buffsize;
	m_curByte = 0;
	m_buffByte = 0;
	m_curPos = 1;
	m_ownsFile = true;
	m_curLine = 1;
	m_original = true;
	return true;
}

long CParseFile::GetRemainingSize()
{
	return m_filesize - m_curByte;
}

bool CParseFile::NextChar(byte& theByte)
{
	if (m_curByte >= m_filesize)
	{
		return false;
	}
	if (m_buff[m_buffByte] == '\n')
	{
		m_curLine += 1;
		m_curPos = 1;
	}
	else
	{
		m_curPos++;
	}
	theByte = m_buff[m_buffByte++];
	m_curByte++;
	if (m_buffByte >= m_buffsize)
	{
		if (m_buffsize > GetRemainingSize())
		{
			m_buffsize = GetRemainingSize();
		}
		Read(m_buff, m_buffsize);
		m_buffByte = 0;
	}
	return true;
}

int CParseFile::GetCurLine()
{
	return m_curLine;
}

void CParseFile::GetCurFilename(char** theBuff)
{
	*theBuff = NULL;
	if (m_fileName != NULL)
	{
		*theBuff = (char*)malloc(strlen(m_fileName) + 1);
		strcpy(*theBuff, m_fileName);
	}
}

//
// CParseMemory
//

CParseMemory::CParseMemory()
{
}

CParseMemory::~CParseMemory()
{
}

CParseMemory* CParseMemory::Create(byte* data, long datasize, bool original)
{
	CParseMemory* curParse = new CParseMemory();
	curParse->Init(data, datasize, original);
	return curParse;
}

void CParseMemory::Delete()
{
	delete this;
}

bool CParseMemory::CanEndOfFile()
{
	return true;
}

bool CParseMemory::NextChar(byte& theByte)
{
	if (m_offset < m_datasize)
	{
		if (m_data[m_offset] == '\n')
		{
			m_curLine += 1;
			m_curPos = 1;
		}
		else
		{
			m_curPos++;
		}
		theByte = m_data[m_offset++];
		return true;
	}
	else
	{
		return false;
	}
}

void CParseMemory::Init(byte* data, long datasize, bool original)
{
	m_data = data;
	m_curLine = 1;
	m_curPos = 1;
	m_offset = 0;
	m_datasize = datasize;
	m_original = original;
}

long CParseMemory::GetRemainingSize()
{
	return m_datasize - m_offset;
}

int CParseMemory::GetCurLine()
{
	return m_curLine;
}

void CParseMemory::GetCurFilename(char** theBuff)
{
	*theBuff = NULL;
}

//
// CParseBlock
//

CParseBlock::CParseBlock()
{
}

CParseBlock::~CParseBlock()
{
}

CParseBlock* CParseBlock::Create(byte* data, long datasize)
{
	CParseBlock* curParse = new CParseBlock();
	curParse->Init(data, datasize);
	return curParse;
}

void CParseBlock::Delete()
{
	if (m_data != NULL)
	{
		free(m_data);
		m_data = NULL;
	}
	delete this;
}

bool CParseBlock::CanEndOfFile()
{
	return false;
}

void CParseBlock::Init(byte* data, long datasize)
{
	m_data = (byte*)malloc(datasize);
	memcpy(m_data, data, datasize);
	m_curLine = 1;
	m_curPos = 1;
	m_offset = 0;
	m_datasize = datasize;
	m_original = false;
}

//
// CParseToken
//

CParseToken::CParseToken()
{
}

CParseToken::~CParseToken()
{
}

CParseToken* CParseToken::Create(CToken* token)
{
	CParseToken* curParse = new CParseToken();
	curParse->Init(token);
	return curParse;
}

void CParseToken::Delete()
{
	if (m_data != NULL)
	{
		free(m_data);
		m_data = NULL;
	}
	delete this;
}

bool CParseToken::NextChar(byte& theByte)
{
	if (m_offset < m_datasize)
	{
		if (m_data[m_offset] == '\n')
		{
			m_curLine += 1;
			m_curPos = 1;
		}
		else
		{
			m_curPos++;
		}
		theByte = m_data[m_offset++];
		return true;
	}
	else
	{
		return false;
	}
}

void CParseToken::Init(CToken* token)
{
	LPCTSTR tokenString = token->GetStringValue();
	m_datasize = strlen(tokenString);
	if (m_datasize > 0)
	{
		m_data = (byte*) malloc(m_datasize);
		memcpy(m_data, tokenString, m_datasize);
	}
	else
	{
		m_data = NULL;
	}
	m_curLine = 1;
	m_curPos = 1;
	m_offset = 0;
	m_original = false;
	token->Delete();
}

long CParseToken::GetRemainingSize()
{
	return m_datasize - m_offset;
}

int CParseToken::GetCurLine()
{
	return m_curLine;
}

void CParseToken::GetCurFilename(char** theBuff)
{
	*theBuff = NULL;
}

//
// CParseDefine
//

CParseDefine::CParseDefine()
{
}

CParseDefine::~CParseDefine()
{
}

CParseDefine* CParseDefine::Create(CDirectiveSymbol* definesymbol)
{
	CParseDefine* retval = new CParseDefine();
	retval->Init(definesymbol);
	return retval;
}

bool CParseDefine::CanEndOfFile()
{
	return false;
}

void CParseDefine::Delete()
{
	CParseMemory::Delete();
}

void CParseDefine::Init(CDirectiveSymbol* definesymbol)
{
	CParseMemory::Init((byte*)definesymbol->GetValue(), strlen(definesymbol->GetValue()));
	m_defineSymbol = definesymbol;
}

bool CParseDefine::IsThisDefinition(void* theDefinition)
{
	return (CDirectiveSymbol*)theDefinition == m_defineSymbol;
}

//
// CToken
//

CToken::CToken()
{
}

CToken::~CToken()
{
}

CToken* CToken::Create()
{
	CToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = theToken->GetNext();
	}
	theToken->Init();
	return theToken;
}

void CToken::Delete()
{
	if (m_string != NULL)
	{
		free(m_string);
		m_string = NULL;
	}
	MakeAvail();
}

void CToken::Init()
{
	m_next = NULL;
	m_string = NULL;
}

void CToken::SetNext(CToken* theToken)
{
	m_next = theToken;
}

CToken* CToken::GetNext()
{
	return m_next;
}

int CToken::GetType()
{
	return TK_EOF;
}

int CToken::GetIntValue()
{
	return 0;
}

LPCTSTR CToken::GetStringValue()
{
	if (m_string == NULL)
	{
		return "";
	}
	return m_string;
}

float CToken::GetFloatValue()
{
	return 0.0;
}

//
// CCharToken
//

CCharToken::CCharToken()
{
}

CCharToken::~CCharToken()
{
}

CCharToken* CCharToken::Create(byte theByte)
{
	CCharToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CCharToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CCharToken*)theToken->GetNext();
	}
	theToken->Init(theByte);
	return theToken;
}

void CCharToken::Delete()
{
	CToken::Delete();
}

void CCharToken::Init(byte theByte)
{
	CToken::Init();
	char charString[10];
	switch(theByte)
	{
		case '\0':
			strcpy(charString, "\\0");
			break;
		case '\n':
			strcpy(charString, "\\n");
			break;
		case '\\':
			strcpy(charString, "\\\\");
			break;
		case '\'':
			strcpy(charString, "\\'");
			break;
		case '\?':
			strcpy(charString, "\\?");
			break;
		case '\a':
			strcpy(charString, "\\a");
			break;
		case '\b':
			strcpy(charString, "\\b");
			break;
		case '\f':
			strcpy(charString, "\\f");
			break;
		case '\r':
			strcpy(charString, "\\r");
			break;
		case '\t':
			strcpy(charString, "\\t");
			break;
		case '\v':
			strcpy(charString, "\\v");
			break;
		default:
			charString[0] = (char)theByte;
			charString[1] = '\0';
			break;
	}
	m_string = (char*)malloc(strlen(charString) + 1);
	strcpy(m_string, charString);
}

int CCharToken::GetType()
{
	return TK_CHAR;
}

//
// CStringToken
//

CStringToken::CStringToken()
{
}

CStringToken::~CStringToken()
{
}

CStringToken* CStringToken::Create(LPCTSTR theString)
{
	CStringToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CStringToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CStringToken*)theToken->GetNext();
	}
	theToken->Init(theString);
	return theToken;
}

void CStringToken::Delete()
{
	CToken::Delete();
}

void CStringToken::Init(LPCTSTR theString)
{
	CToken::Init();
	m_string = (char*)malloc(strlen(theString) + 1);
//	ASSERT(m_string);
	strcpy(m_string, theString);
}

int CStringToken::GetType()
{
	return TK_STRING;
}

//
// CIntToken
//

CIntToken::CIntToken()
{
}

CIntToken::~CIntToken()
{
}

CIntToken* CIntToken::Create(long value)
{
	CIntToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CIntToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CIntToken*)theToken->GetNext();
	}
	theToken->Init(value);
	return theToken;
}

void CIntToken::Delete()
{
	CToken::Delete();
}

void CIntToken::Init(long value)
{
	CToken::Init();
	m_value = value;
}

int CIntToken::GetType()
{
	return TK_INT;
}

int CIntToken::GetIntValue()
{
	return m_value;
}

float CIntToken::GetFloatValue()
{
	return (float)m_value;
}

LPCTSTR CIntToken::GetStringValue()
{
	if (m_string != NULL)
	{
		free(m_string);
		m_string = NULL;
	}
	char temp[128];
	sprintf(temp, "%d", m_value);
	m_string = (char*)malloc(strlen(temp) + 1);
	strcpy(m_string, temp);
	return m_string;
}

//
// CFloatToken
//

CFloatToken::CFloatToken()
{
}

CFloatToken::~CFloatToken()
{
}

CFloatToken* CFloatToken::Create(float value)
{
	CFloatToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CFloatToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CFloatToken*)theToken->GetNext();
	}
	theToken->Init(value);
	return theToken;
}

void CFloatToken::Delete()
{
	CToken::Delete();
}

void CFloatToken::Init(float value)
{
	CToken::Init();
	m_value = value;
}

int CFloatToken::GetType()
{
	return TK_FLOAT;
}

float CFloatToken::GetFloatValue()
{
	return m_value;
}

LPCTSTR CFloatToken::GetStringValue()
{
	if (m_string != NULL)
	{
		free(m_string);
		m_string = NULL;
	}
	char temp[128];
	sprintf(temp, "%g", m_value);
	m_string = (char*)malloc(strlen(temp) + 1);
	strcpy(m_string, temp);
	return m_string;
}

//
// CIdentifierToken
//

CIdentifierToken::CIdentifierToken()
{
}

CIdentifierToken::~CIdentifierToken()
{
}

CIdentifierToken* CIdentifierToken::Create(LPCTSTR name)
{
	CIdentifierToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CIdentifierToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CIdentifierToken*)theToken->GetNext();
	}
	theToken->Init(name);
	return theToken;
}

void CIdentifierToken::Delete()
{
	CToken::Delete();
}

void CIdentifierToken::Init(LPCTSTR name)
{
	CToken::Init();
	m_string = (char*)malloc(strlen(name) + 1);
//	ASSERT(m_string);
	strcpy(m_string, name);
}

int CIdentifierToken::GetType()
{
	return TK_IDENTIFIER;
}

//
// CCommentToken
//

CCommentToken::CCommentToken()
{
}

CCommentToken::~CCommentToken()
{
}

CCommentToken* CCommentToken::Create(LPCTSTR name)
{
	CCommentToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CCommentToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CCommentToken*)theToken->GetNext();
	}
	theToken->Init(name);
	return theToken;
}

void CCommentToken::Delete()
{
	CToken::Delete();
}

void CCommentToken::Init(LPCTSTR name)
{
	CToken::Init();
	m_string = (char*)malloc(strlen(name) + 1);
//	ASSERT(m_string);
	strcpy(m_string, name);
}

int CCommentToken::GetType()
{
	return TK_COMMENT;
}

//
// CSpaceToken
//

CSpaceToken::CSpaceToken()
{
}

CSpaceToken::~CSpaceToken()
{
}

CSpaceToken* CSpaceToken::Create(LPCTSTR name)
{
	CSpaceToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CSpaceToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CSpaceToken*)theToken->GetNext();
	}
	theToken->Init(name);
	return theToken;
}

void CSpaceToken::Delete()
{
	CToken::Delete();
}

void CSpaceToken::Init(LPCTSTR name)
{
	CToken::Init();
	m_string = (char*)malloc(strlen(name) + 1);
//	ASSERT(m_string);
	strcpy(m_string, name);
}

int CSpaceToken::GetType()
{
	return TK_SPACE;
}

//
// CUserToken
//

CUserToken::CUserToken()
{
}

CUserToken::~CUserToken()
{
}

CUserToken* CUserToken::Create(int value, LPCTSTR string)
{
	CUserToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CUserToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CUserToken*)theToken->GetNext();
	}
	theToken->Init(value, string);
	return theToken;
}

void CUserToken::Delete()
{
	CToken::Delete();
}

void CUserToken::Init(int value, LPCTSTR string)
{
	CToken::Init();
	m_value = value;
	m_string = (char*)malloc(strlen(string) + 1);
	strcpy(m_string, string);
}

int CUserToken::GetType()
{
	return m_value;
}

//
// CUndefinedToken
//

CUndefinedToken::CUndefinedToken()
{
}

CUndefinedToken::~CUndefinedToken()
{
}

CUndefinedToken* CUndefinedToken::Create(LPCTSTR string)
{
	CUndefinedToken* theToken;
	if (m_avail == NULL)
	{
		theToken = new CUndefinedToken();
	}
	else
	{
		theToken = m_avail;
		m_avail = (CUndefinedToken*)theToken->GetNext();
	}
	theToken->Init(string);
	return theToken;
}

void CUndefinedToken::Delete()
{
	CToken::Delete();
}

void CUndefinedToken::Init(LPCTSTR string)
{
	CToken::Init();
	m_string = (char*)malloc(strlen(string) + 1);
	strcpy(m_string, string);
}

int CUndefinedToken::GetType()
{
	return TK_UNDEFINED;
}

//
// CTokenizerState
//

CTokenizerState::CTokenizerState()
{
}

CTokenizerState::~CTokenizerState()
{
}

CTokenizerState* CTokenizerState::Create(bool skip)
{
	CTokenizerState* retval = new CTokenizerState();
	retval->Init(skip);
	return retval;
}

void CTokenizerState::Init(bool skip)
{
	m_next = NULL;
	m_skip = skip;
	m_elseHit = false;
}

void CTokenizerState::Delete()
{
	delete this;
}

CTokenizerState* CTokenizerState::GetNext()
{
	return m_next;
}

bool CTokenizerState::ProcessElse()
{
	if (!m_elseHit)
	{
		m_elseHit = true;
		m_skip = !m_skip;
	}
	return m_elseHit;
}

void CTokenizerState::SetNext(CTokenizerState* next)
{
	m_next = next;
}

bool CTokenizerState::Skipping()
{
	return m_skip;
}

//
// CTokenizerHolderState
//

CTokenizerHolderState::CTokenizerHolderState()
{
}

CTokenizerHolderState::~CTokenizerHolderState()
{
}

CTokenizerHolderState* CTokenizerHolderState::Create()
{
	CTokenizerHolderState* retval = new CTokenizerHolderState();
	retval->Init();
	return retval;
}

void CTokenizerHolderState::Init()
{
	CTokenizerState::Init(true);
}

void CTokenizerHolderState::Delete()
{
	delete this;
}

bool CTokenizerHolderState::ProcessElse()
{
	if (!m_elseHit)
	{
		m_elseHit = true;
	}
	return m_elseHit;
}

//
// CKeywordTable
//

CKeywordTable::CKeywordTable(CTokenizer* tokenizer, keywordArray_t* keywords)
{
	m_tokenizer = tokenizer;
	m_holdKeywords = tokenizer->SetKeywords(keywords);
}

CKeywordTable::~CKeywordTable()
{
	m_tokenizer->SetKeywords(m_holdKeywords);
}

//
// CTokenizer
//

CTokenizer::CTokenizer()
{
}

CTokenizer::~CTokenizer()
{
}

CTokenizer* CTokenizer::Create(UINT dwFlags)
{
	CTokenizer* theTokenizer = new CTokenizer();
	theTokenizer->Init(dwFlags);
	return theTokenizer;
}

void CTokenizer::Delete()
{
	while (m_curParseStream != NULL)
	{
		CParseStream* curStream = m_curParseStream;
		m_curParseStream = curStream->GetNext();
		curStream->Delete();
	}
	if (m_symbolLookup != NULL)
	{
		m_symbolLookup->Delete();
		m_symbolLookup = NULL;
	}
	while (m_nextToken != NULL)
	{
		CToken* curToken = m_nextToken;
		m_nextToken = curToken->GetNext();
		curToken->Delete();
	}
	while (m_state != NULL)
	{
		Error(TKERR_UNMATCHED_DIRECTIVE);
		CTokenizerState* curState = m_state;
		m_state = curState->GetNext();
		curState->Delete();
	}

/*	if (m_lastErrMsg != NULL)
	{
		free(m_lastErrMsg);
		m_lastErrMsg = NULL;
	}*/
	CToken::DeleteAll();
	CCharToken::DeleteAll();
	CIntToken::DeleteAll();
	CFloatToken::DeleteAll();
	CStringToken::DeleteAll();
	CSpaceToken::DeleteAll();
	CUndefinedToken::DeleteAll();
	CUserToken::DeleteAll();
	CIdentifierToken::DeleteAll();
	CCommentToken::DeleteAll();
	delete this;
}

void CTokenizer::Error(int theError)
{
	char errString[128];
	char lookupstring[128];
	int i = 0;
	while ((errorMessages[i].m_tokenvalue != TKERR_USERERROR) && (errorMessages[i].m_tokenvalue != theError))
	{
		i++;
	}
	if ((errorMessages[i].m_tokenvalue == TKERR_USERERROR) && (m_errors != NULL))
	{
		i = 0;
		while ((m_errors[i].m_tokenvalue != TK_EOF) && (m_errors[i].m_tokenvalue != theError))
		{
			i++;
		}
		strcpy(lookupstring, m_errors[i].m_keyword);
	}
	else
	{
		strcpy(lookupstring, errorMessages[i].m_keyword);
	}
	
	// If anyone objects to me altering this, say so... -Ste
//	sprintf(errString, "Error -- %d, %s", theError, lookupstring);	
	sprintf(errString, "Error -- %d, \"%s\" (line %d)", theError, lookupstring, GetCurLine());	
	Error(errString, theError);
}

void CTokenizer::Error(int theError, LPCTSTR errString)
{
	char errstring[128];
	char lookupstring[128];
	int i = 0;
	while ((errorMessages[i].m_tokenvalue != TKERR_USERERROR) && (errorMessages[i].m_tokenvalue != theError))
	{
		i++;
	}
	if ((errorMessages[i].m_tokenvalue == TKERR_USERERROR) && (m_errors != NULL))
	{
		i = 0;
		while ((m_errors[i].m_tokenvalue != TK_EOF) && (m_errors[i].m_tokenvalue != theError))
		{
			i++;
		}
		strcpy(lookupstring, m_errors[i].m_keyword);
	}
	else
	{
		strcpy(lookupstring, errorMessages[i].m_keyword);
	}

	// If anyone objects to me altering this, say so... -Ste
//	sprintf(errstring, "Error -- %d, %s - %s", theError, lookupstring, errString);
	_snprintf(errstring, sizeof(errstring), "Error -- %d, %s - \"%s\" (line %d)", theError, lookupstring, errString, GetCurLine());
	Error(errstring, theError);
}

void CTokenizer::Error(LPCTSTR errString, int theError)
{
	if (m_errorProc != NULL)
	{
		m_errorProc(errString);
	}
#ifdef USES_MODULES
	else 
	{
		ReportError(theError, errString);
	}
#endif
}

bool CTokenizer::AddParseFile(LPCTSTR filename, DWORD buffsize)
{
	CParseStream* newStream = CParseFile::Create(filename, this, buffsize);

	if ( newStream != NULL )
	{
		newStream->SetNext(m_curParseStream);
		m_curParseStream = newStream;
		return true;
	}

	return false;
}

void CTokenizer::AddParseStream(byte* data, long datasize)
{
	CParseStream* newStream = CParseMemory::Create(data, datasize, true);
	newStream->SetNext(m_curParseStream);
	m_curParseStream = newStream;
}

long CTokenizer::GetRemainingSize()
{
	long retval = 0;
	CParseStream* curStream = m_curParseStream;
	while (curStream != NULL)
	{
		retval += curStream->GetRemainingSize();
		curStream = curStream->GetNext();
	}
	return retval;
}

LPCTSTR CTokenizer::LookupToken(int tokenID, keywordArray_t* theTable)
{
	if (theTable == NULL)
	{
		theTable = m_keywords;
	}
	if (theTable == NULL)
	{
		return NULL;
	}

	int i = 0;
	while (theTable[i].m_tokenvalue != TK_EOF)
	{
		if (theTable[i].m_tokenvalue == tokenID)
		{
			return theTable[i].m_keyword;
		}
		i++;
	}
	return NULL;
}

void CTokenizer::PutBackToken(CToken* theToken, bool commented, LPCTSTR addedChars)
{
	if (commented)
	{
		CParseToken* newStream = CParseToken::Create(theToken);
		newStream->SetNext(m_curParseStream);
		m_curParseStream = newStream;

		if (addedChars != NULL)
		{
			CParsePutBack* spacer = CParsePutBack::Create(' ', newStream);
			spacer->SetNext(m_curParseStream);
			m_curParseStream = spacer;

			CParseBlock* newBlock = CParseBlock::Create((byte*)addedChars, strlen(addedChars));
			newBlock->SetNext(m_curParseStream);
			m_curParseStream = newBlock;
		}

		char temp[] = "// * ";
		CParseBlock* newBlock = CParseBlock::Create((byte*)temp, strlen(temp));
		newBlock->SetNext(m_curParseStream);
		m_curParseStream = newBlock;
		return;
	}

	switch(theToken->GetType())
	{
	case TK_INT:
	case TK_EOF:
	case TK_UNDEFINED:
	case TK_FLOAT:
	case TK_CHAR:
	case TK_STRING:
	case TK_EOL:
	case TK_COMMENT:
		theToken->SetNext(m_nextToken);
		m_nextToken = theToken;
		break;
	default:
		CParseToken* newStream = CParseToken::Create(theToken);
		newStream->SetNext(m_curParseStream);
		m_curParseStream = newStream;
		break;
	}

	if (addedChars != NULL)
	{
		CParseBlock* newBlock = CParseBlock::Create((byte*)addedChars, strlen(addedChars));
		newBlock->SetNext(m_curParseStream);
		m_curParseStream = newBlock;
	}
}

CToken* CTokenizer::GetToken(keywordArray_t* keywords, UINT onFlags, UINT offFlags)
{
	keywordArray_t* holdKeywords = SetKeywords(keywords);
	CToken* retval = GetToken(onFlags, offFlags);
	SetKeywords(holdKeywords);
	return retval;
}

CToken* CTokenizer::GetToken(UINT onFlags, UINT offFlags)
{
	UINT holdFlags = m_flags;

	m_flags |= onFlags;
	m_flags &= (~offFlags);
	CToken* theToken = NULL;
	while (theToken == NULL)
	{
		theToken = FetchToken();
		if (theToken == NULL)
		{
			continue;
		}
		if (theToken->GetType() == TK_EOF)
		{
			break;
		}
		if (m_state != NULL)
		{
			if (m_state->Skipping())
			{
				theToken->Delete();
				theToken = NULL;
			}
		}
	}

	m_flags = holdFlags;
	return theToken;
}

void CTokenizer::GetToEndOfFile()
{
	m_parsingToEnd = true;
	byte theByte;
	while(m_parsingToEnd)
	{
		if (!NextChar(theByte))
		{
			return;
		}
	}
}

CToken* CTokenizer::GetToEndOfLine(int tokenType)
{
	char theString[MAX_STRING_LENGTH];
	theString[0] = ' ';
	while (isspace(theString[0]))
	{
		if (!NextChar((byte&)theString[0]))
		{
			return NULL;
		}
		if ((theString[0] == '\n') || (theString[0] == '\r'))
		{
			PutBackChar(theString[0]);
			theString[0] = ' ';
			break;
		}
	}
	for (int i = 1; ; i++)
	{
		char *read;
		
		if (i < MAX_STRING_LENGTH)
			read = &theString[i];
		else
			read = &theString[MAX_STRING_LENGTH-1];	//just use the last char

		if (NextChar((byte&)*read))
		{
			if ((*read != '\n') && (*read != '\r'))
			{
				continue;
			}
			PutBackChar(*read);
		}
		
		//got to end of line
		*read = '\0';

		switch(tokenType)
		{
		case TK_COMMENT:
			//
			// strip trailing whitespace...
			//
			while (strlen(theString) && isspace(theString[strlen(theString)-1]))
			{
				theString[strlen(theString)-1] = 0;
			}
			if (i >= MAX_STRING_LENGTH)
				Error(TKERR_STRINGLENGTHEXCEEDED);
			return CCommentToken::Create(theString);
		case TK_IDENTIFIER:
		default:
			if (i >= MAX_STRING_LENGTH)
				Error(TKERR_IDENTIFIERLENGTHEXCEEDED);
			return CIdentifierToken::Create(theString);
		}
	}
}

void CTokenizer::SkipToLineEnd()
{
	byte theByte;
	while(NextChar(theByte))
	{
		if (theByte == '\n')
		{
			break;
		}
	}
}

CToken* CTokenizer::FetchToken()
{
	if (m_nextToken != NULL)
	{
		CToken* curToken = m_nextToken;
		m_nextToken = curToken->GetNext();
		curToken->SetNext(NULL);
		return curToken;
	}
	byte theByte;
	CToken* theToken = NULL;

	while (true)
	{
		if (!NextChar(theByte))
		{
			if ((m_curParseStream != NULL) && ((TKF_RETURN_EOF & m_flags) != 0))
			{
				CParseStream* curParseStream = m_curParseStream;
				m_curParseStream = curParseStream->GetNext();
				curParseStream->Delete();
			}
			return CToken::Create();
		}
		if ((theByte == ' ') && (m_flags & TKF_SPACETOKENS))
		{
			return CSpaceToken::Create(" ");
		}
		if (theByte <= ' ')
		{
			if ((theByte == '\n') && ((TKF_USES_EOL & m_flags) != 0))
			{
				return CUserToken::Create(TK_EOL, "-EOLN-");
			}
			continue;
		}
		switch(theByte)
		{
		case '#':
			if ((m_flags & TKF_IGNOREDIRECTIVES) == 0)
			{
				theToken = HandleDirective();
			}
			else
			{
				if ((m_flags & TKF_NODIRECTIVES) != 0)
				{
					return HandleSymbol('#');
				}
				SkipToLineEnd();
			}
			break;
		case '/':
			theToken = HandleSlash();
			break;
		case '"':
			theToken = HandleString();
			break;
		case '\'':
			theToken = HandleQuote();
			break;
		default:
			if (((theByte >= 'a') && (theByte <= 'z'))
				|| ((theByte >= 'A') && (theByte <= 'Z'))
				|| ((theByte == '_') && ((m_flags & TKF_NOUNDERSCOREINIDENTIFIER) == 0)))
			{
				theToken = HandleIdentifier(theByte);
			}
			else if (((m_flags & TKF_RETURNINTSASIDENTIFIERS) != 0) && (theByte >= '0') && (theByte <= '9'))
			{
				theToken = HandleIdentifier(theByte);
			}
			else if (((m_flags & TKF_NUMERICIDENTIFIERSTART) != 0) && (theByte >= '0') && (theByte <= '9'))
			{
				theToken = HandleIdentifier(theByte);
			}
			else if (((theByte >= '0') && (theByte <= '9')) || (theByte == '-'))
			{
				theToken = HandleNumeric(theByte);
			}
			else if (theByte == '.')
			{
				theToken = HandleDecimal();
			}
			else if (theByte <= ' ')
			{
				break;
			}
			else
			{
				theToken = HandleSymbol(theByte);
			}
		}
		if (theToken != NULL)
		{
			return theToken;
		}
	}
}

bool CTokenizer::NextChar(byte& theByte)
{
	while (m_curParseStream != NULL)
	{
		if (m_curParseStream->NextChar(theByte))
		{
			return true;
		}
		bool endoffile = m_curParseStream->CanEndOfFile();
		if (endoffile && ((TKF_RETURN_EOF & m_flags) != 0))
		{
			return false;
		}
		CParseStream* curParseStream = m_curParseStream;
		m_curParseStream = curParseStream->GetNext();
		curParseStream->Delete();
		if (m_parsingToEnd && endoffile)
		{
			break;
		}
	}
	if (m_parsingToEnd)
	{
		theByte = TK_EOF;
		m_parsingToEnd = false;
		return true;
	}
	return false;
}

bool CTokenizer::RequireToken(int tokenType)
{
	CToken* theToken = GetToken();
	bool retValue = theToken->GetType() == tokenType;
	theToken->Delete();
	return retValue;
}

void CTokenizer::ScanUntilToken(int tokenType)
{
	CToken* curToken;
	int tokenValue = TK_UNDEFINED;
	while (tokenValue != tokenType)
	{
		curToken = GetToken();
		tokenValue = curToken->GetType();
		if (tokenValue == TK_EOF)
		{
			PutBackToken(curToken);
			break;
		}
		if (tokenValue == tokenType)
		{
			PutBackToken(curToken);
		}
		else
		{
			curToken->Delete();
		}
	}
}

void CTokenizer::Init(UINT dwFlags)
{
	m_symbolLookup = NULL;
	m_nextToken = NULL;
	m_curParseStream = NULL;
	m_keywords = NULL;
	m_symbols = NULL;
	m_errors = NULL;
	m_state = NULL;
	m_flags = dwFlags;
	m_errorProc = NULL;
	m_parsingToEnd = false;
}

void CTokenizer::SetErrorProc(LPTokenizerErrorProc errorProc)
{
	m_errorProc = errorProc;
}

void CTokenizer::SetAdditionalErrors(keywordArray_t* theErrors)
{
	m_errors = theErrors;
}

keywordArray_t* CTokenizer::SetKeywords(keywordArray_t* theKeywords)
{
	keywordArray_t* retval = m_keywords;
	m_keywords = theKeywords;
	return retval;
}

void CTokenizer::SetSymbols(keywordArray_t* theSymbols)
{
	m_symbols = theSymbols;
	if (m_symbolLookup != NULL)
	{
		m_symbolLookup->Delete();
		m_symbolLookup = NULL;
	}
	int i = 0;
	if (theSymbols == NULL)
	{
		return;
	}
	while(theSymbols[i].m_tokenvalue != TK_EOF)
	{
		InsertSymbol(theSymbols[i].m_keyword, theSymbols[i].m_tokenvalue);
		i++;
	}
}

void CTokenizer::InsertSymbol(LPCTSTR theSymbol, int theValue)
{
	CSymbolLookup** curHead = &m_symbolLookup;
	CSymbolLookup* curParent = NULL;
	CSymbolLookup* curLookup = NULL;

	for (UINT i = 0; i < strlen(theSymbol); i++)
	{
		bool found = false;
		curLookup = *curHead;
		while (curLookup != NULL)
		{
			if (curLookup->GetByte() == theSymbol[i])
			{
				found = true;
				break;
			}
			curLookup = curLookup->GetNext();
		}
		if (!found)
		{
			curLookup = CSymbolLookup::Create(theSymbol[i]);
			curLookup->SetParent(curParent);
			curLookup->SetNext(*curHead);
			*curHead = curLookup;
		}
		curHead = curLookup->GetChildAddress();
		curParent = curLookup;
	}
	if (curLookup->GetValue() != -1)
	{
		Error(TKERR_DUPLICATESYMBOL);
	}
	curLookup->SetValue(theValue);
}

CToken* CTokenizer::HandleString()
{
	char theString[MAX_STRING_LENGTH];
	for (int i = 0; i < MAX_STRING_LENGTH; i++)
	{
		if (!NextChar((byte&)theString[i]))
		{
			return NULL;
		}
		if (theString[i] == '"')
		{
			theString[i] = '\0';
			return CStringToken::Create(theString);
		}
		if (theString[i] == '\\')
		{
			theString[i] = Escapement();
		}
	}
	Error(TKERR_STRINGLENGTHEXCEEDED);
	return NULL;
}

void CTokenizer::GetCurFilename(char** filename)
{
	if (m_curParseStream == NULL)
	{
		*filename = (char*)malloc(1);
		*filename[0] = '\0';
		return;
	}
	m_curParseStream->GetCurFilename(filename);
}

int CTokenizer::GetCurLine()
{
	if (m_curParseStream == NULL)
	{
		return 0;
	}

	return m_curParseStream->GetCurLine();
}

void CTokenizer::PutBackChar(byte theByte)
{
	CParseStream* newStream = CParsePutBack::Create(theByte, m_curParseStream);
	newStream->SetNext(m_curParseStream);
	m_curParseStream = newStream;
}

byte CTokenizer::Escapement()
{
	byte theByte;
	if (NextChar(theByte))
	{
		if (m_flags & TKF_NO_ESC_XLAT)
		{
			PutBackChar(theByte);
		}
		else
		{
			switch(theByte)
			{
			case 'n':
				return '\n';
			case '\\':
				return '\\';
			case '\'':
				return '\'';
			case '?':
				return '\?';
			case 'a':
				return '\a';
			case 'b':
				return '\b';
			case 'f':
				return '\f';
			case 'r':
				return '\r';
			case 't':
				return '\t';
			case 'v':
				return '\v';
			case '0':
				return '\0';
			// support for octal or hex sequences?? \000 or \xhhh
			default:
				PutBackChar(theByte);
			}
		}
	}
	return '\\';
}

bool CTokenizer::AddDefineSymbol(CDirectiveSymbol* definesymbol)
{
	CParseStream* curStream = m_curParseStream;
	while(curStream != NULL)
	{
		if (curStream->IsThisDefinition(definesymbol))
		{
			return false;
		}
		curStream = curStream->GetNext();
	}
	CParseStream* newStream = CParseDefine::Create(definesymbol);
	newStream->SetNext(m_curParseStream);
	m_curParseStream = newStream;
	return true;
}

CToken* CTokenizer::TokenFromName(LPCTSTR name)
{
	CDirectiveSymbol* defineSymbol = (CDirectiveSymbol*)m_defines.FindSymbol(name);
	if (defineSymbol != NULL)
	{
		if (AddDefineSymbol(defineSymbol))
		{
			return FetchToken();
		}
	}
	if ((m_keywords != NULL) && ((m_flags & TKF_IGNOREKEYWORDS) == 0))
	{
		int i = 0;
		if ((m_flags & TKF_NOCASEKEYWORDS) == 0)
		{
			while (m_keywords[i].m_tokenvalue != TK_EOF)
			{
				if (strcmp(m_keywords[i].m_keyword, name) == 0)
				{
					return CUserToken::Create(m_keywords[i].m_tokenvalue, name);
				}
				i++;
			}
		}
		else
		{
			while (m_keywords[i].m_tokenvalue != TK_EOF)
			{
				if (stricmp(m_keywords[i].m_keyword, name) == 0)
				{
					return CUserToken::Create(m_keywords[i].m_tokenvalue, name);
				}
				i++;
			}
		}
	}
	return CIdentifierToken::Create(name);
}

int CTokenizer::DirectiveFromName(LPCTSTR name)
{
	if (directiveKeywords != NULL)
	{
		int i = 0;
		while (directiveKeywords[i].m_tokenvalue != TK_EOF)
		{
			if (strcmp(directiveKeywords[i].m_keyword, name) == 0)
			{
				return directiveKeywords[i].m_tokenvalue;
			}
			i++;
		}
	}
	return -1;
}

CToken* CTokenizer::HandleIdentifier(byte theByte)
{
	char theString[MAX_IDENTIFIER_LENGTH];
	theString[0] = theByte;
	bool numeric = (theByte >= '0') && (theByte <= '9');
	for (int i = 1; i < MAX_IDENTIFIER_LENGTH; i++)
	{
		if (NextChar((byte&)theString[i]))
		{
			if (((theString[i] != '_') || ((m_flags & TKF_NOUNDERSCOREINIDENTIFIER) == 0)) &&
				((theString[i] != '-') || ((m_flags & TKF_NODASHINIDENTIFIER) == 0)))
			{
				if ((theString[i] >= '0') && (theString[i] <= '9'))
				{
					continue;
				}
				else
				if (((theString[i] >= 'A') && (theString[i] <= 'Z'))
				 || ((theString[i] >= 'a') && (theString[i] <= 'z'))
				 || (theString[i] == '_') || (theString[i] == '-'))
				{
					numeric = false;
					continue;
				}
			}
			if ((numeric) && ((m_flags & TKF_RETURNINTSASIDENTIFIERS) == 0))
			{
				theByte = theString[i];
				theString[i] = '\0';
				if (theByte == '.')
				{
					long value = atoi(theString);
					return HandleFloat(false, value);
				}
				PutBackChar(theByte);
				i--;
				theByte = theString[i];
				theString[i] = '\0';
				long value = atoi(theString);
				return HandleNumeric(theByte, value);
			}
			PutBackChar(theString[i]);
		}
		theString[i] = '\0';
		return TokenFromName(theString);
	}
	Error(TKERR_IDENTIFIERLENGTHEXCEEDED);
	return NULL;
}

CToken* CTokenizer::HandleSlash()
{
	byte theByte;
	if (!NextChar(theByte))
	{
		return NULL;
	}
	if (theByte == '/')
	{
		if (m_flags & (TKF_COMMENTTOKENS|TKF_COMMENTTOKENSSLASHSLASHONLY))
		{
			return GetToEndOfLine(TK_COMMENT);
		}
		SkipToLineEnd();
		return NULL;
	}
	if (theByte == '*')
	{
		if ( (m_flags & (TKF_COMMENTTOKENS|TKF_COMMENTTOKENSSLASHSLASHONLY)) == TKF_COMMENTTOKENS )
		{
			char theString[MAX_IDENTIFIER_LENGTH + 1];
			memset(theString,0,sizeof(theString));	// needed later
			theString[0] = ' ';
			while (isspace(theString[0]))
			{
				if (!NextChar((byte&)theString[0]))
				{
					return NULL;
				}
			}			
			for (int i = 1; i < MAX_IDENTIFIER_LENGTH; i++)
			{
				if (NextChar((byte&)theString[i]))
				{
					if (theString[i] != '*')
					{
						continue;
					}					
					i++;
					if (NextChar((byte&)theString[i]))
					{
						if (theString[i] == '/')
						{
							i--;
							theString[i] = '\0';
							//
							// strip trailing whitespace...
							//
							while (strlen(theString) && isspace(theString[strlen(theString)-1]))
							{
								theString[strlen(theString)-1] = 0;
							}

							return CCommentToken::Create(theString);
						}
					}
				}
			}
			Error(TKERR_IDENTIFIERLENGTHEXCEEDED);
			//
			// we should actually keep reading until we've finished scanning the /**/ comment completely, even though
			//	we're not returning it, or we end up returning stuff inside this comment as individual tokens later on
			//
			while (NextChar(theByte))
			{
				// note slightly awkward check to see if the leading star was the last char read above (unlikely but possible)
				//
				if (theByte != '*' && theString[MAX_IDENTIFIER_LENGTH-1]!= '*')
				{
					theString[MAX_IDENTIFIER_LENGTH-1]=0;
					continue;
				}				
				if (NextChar(theByte))
				{					
					if (theByte == '/')
					{
						break;
					}
				}
				else
				{
					break;
				}
			}

			return NULL;
		}
		while(NextChar(theByte))
		{
			while(theByte == '*')
			{
				if (!NextChar(theByte))
				{
					break;
				}
				if (theByte == '/')
				{
					return NULL;
				}
			}
		}
		return NULL;
	}
	PutBackChar(theByte);
	return HandleSymbol('/');
}

CToken* CTokenizer::HandleNumeric(byte theByte, long value)
{
	bool thesign = theByte == '-';
	if (thesign)
	{
		if (!NextChar(theByte))
		{
			return HandleSymbol('-');
		}
		if (theByte == '.')
		{
			return HandleDecimal(thesign);
		}
		if ((theByte < '0') || (theByte > '9'))
		{
			PutBackChar(theByte);
			return HandleSymbol('-');
		}
	}
	if (theByte == 'O')
	{
		return HandleOctal(thesign);
	}

	bool digithit = (value != 0);

	if ((theByte == '0') && (value == 0))
	{
		digithit = true;
		if (!NextChar(theByte))
		{
			return CIntToken::Create(0);
		}
		if ((theByte == 'x') || (theByte == 'X'))
		{
			return HandleHex(thesign);
		}
	}
		
	while((theByte >= '0') && (theByte <= '9'))
	{
		value = (value * 10) + (theByte - '0');
		if (!NextChar(theByte))
		{
			if (thesign)
			{
				value = -value;
			}
			return CIntToken::Create(value);
		}
		digithit = true;
	}
	if (theByte == '.')
	{
		if (digithit)
		{
			return HandleFloat(thesign, value);
		}
		if (thesign)
		{
			PutBackChar(theByte);
			theByte = '-';
		}
		return HandleSymbol(theByte);
	}
	PutBackChar(theByte);
	if (thesign)
	{
		value = -value;
	}
	return CIntToken::Create(value);
}

CToken* CTokenizer::HandleOctal(bool thesign)
{
	byte theByte;
	int value = 0;

	if (!NextChar(theByte))
	{
		return CIntToken::Create(value);
	}
	while(true)
	{
		if((theByte >= '0') && (theByte <='7'))
		{
			value = (value * 8) + (theByte - '0');
		}
		else
		{
			PutBackChar(theByte);
			if (thesign)
			{
				value = -value;
			}
			return CIntToken::Create(value);
		}
		if (!NextChar(theByte))
		{
			if (thesign)
			{
				value = -value;
			}
			return CIntToken::Create(value);
		}
	}
}

CToken* CTokenizer::HandleHex(bool thesign)
{
	int value = 0;

	while (true)
	{
		byte theByte;
		if (!NextChar(theByte))
		{
			if (thesign)
			{
				value = -value;
			}
			return CIntToken::Create(value);
		}
		switch (theByte)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			theByte = theByte - '0';
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			theByte = theByte - 'A' + 10;
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			theByte = theByte - 'a' + 10;
			break;
		default:
			PutBackChar(theByte);
			if (thesign)
			{
				value = -value;
			}
			return CIntToken::Create(value);
		}
		value = (value * 16) + theByte;
	}
}

CToken* CTokenizer::HandleDecimal(bool thesign)
{
	byte theByte;
	if (!NextChar(theByte))
	{
		if (thesign)
		{
			PutBackChar('.');
			return HandleSymbol('-');
		}
		HandleSymbol('.');
	}
	PutBackChar(theByte);
	if ((theByte <= '9') && (theByte >= '0'))
	{
		return HandleFloat(thesign);
	}
	if (thesign)
	{
		PutBackChar('.');
		theByte = '-';
	}
	else
	{
		theByte = '.';
	}
	return HandleSymbol(theByte);
}

CToken* CTokenizer::HandleFloat(bool thesign, long value)
{
	float lower = 1.0;
	float newValue = (float)value;
	byte theByte;
	while(NextChar(theByte))
	{
		if ((theByte >= '0') && (theByte <= '9'))
		{
			lower = lower / 10;
			newValue = newValue + ((theByte - '0') * lower);
			continue;
		}
		PutBackChar(theByte);
		break;
	}
	if (thesign)
	{
		newValue = -newValue;
	}
	return CFloatToken::Create(newValue);
}

CToken* CTokenizer::HandleQuote()
{
	byte theByte;
	if (!NextChar(theByte))
	{
		Error(TKERR_EXPECTED_CHAR);
		return NULL;
	}
	if (theByte == '\\')
	{
		theByte = Escapement();
	}
	byte dummy;
	if (!NextChar(dummy))
	{
		Error(TKERR_EXPECTED_CHAR);
		return NULL;
	}
	if (dummy != '\'')
	{
		PutBackChar(dummy);
		PutBackChar(theByte);
		Error(TKERR_EXPECTED_CHAR);
		return NULL;
	}
	return CCharToken::Create(theByte);
}

void CTokenizer::SetFlags(UINT flags)
{
	m_flags = flags;
}

UINT CTokenizer::GetFlags()
{
	return m_flags;
}

CToken* CTokenizer::HandleSymbol(byte theByte)
{
	char symbolString[128];
	int curStrLen = 0;
	symbolString[0] = '\0';
	bool consumed = false;

	CSymbolLookup* curLookup;
	if ((m_flags & TKF_RAWSYMBOLSONLY) == 0)
	{
		curLookup = m_symbolLookup;
	}
	else
	{
		curLookup = NULL;
	}
	CSymbolLookup* lastLookup = NULL;
	while(curLookup != NULL)
	{
		if (curLookup->GetByte() == theByte)
		{
			symbolString[curStrLen++] = theByte;
			symbolString[curStrLen] = '\0';
			lastLookup = curLookup;
			consumed = true;
			if (curLookup->GetChild() == NULL)
			{
				break;
			}
			if (!NextChar(theByte))
			{
				PutBackToken(CToken::Create());
				break;
			}
			consumed = false;
			curLookup = curLookup->GetChild();
			continue;
		}
		curLookup = curLookup->GetNext();
	}
	if ((!consumed) && (lastLookup != NULL))
	{
		PutBackChar(theByte);
	}
	while ((lastLookup != NULL) && (lastLookup->GetValue() == -1))
	{
		curStrLen--;
		symbolString[curStrLen] = '\0';
	//	symbolString = symbolString.Left(symbolString.GetLength() - 1);
		if (lastLookup->GetParent() == NULL)
		{
			if ((m_flags & TKF_WANTUNDEFINED) == 0)
			{
				Error(TKERR_UNRECOGNIZEDSYMBOL);
			}
		}
		else
		{
			PutBackChar(lastLookup->GetByte());
		}
		lastLookup = lastLookup->GetParent();
	}
	if (lastLookup == NULL)
	{
		if ((m_flags & TKF_WANTUNDEFINED) == 0)
		{
			return NULL;
		}
		curStrLen = 0;
		symbolString[curStrLen++] = char(theByte);
		symbolString[curStrLen] = '\0';
		if ((m_flags & TKF_WIDEUNDEFINEDSYMBOLS) != 0)
		{
			while (true)
			{
				if (!NextChar(theByte))
				{
					PutBackToken(CToken::Create());
					break;
				}
				if (theByte == ' ')
				{
					break;
				}
				if (((theByte >= 'a') && (theByte <= 'z')) || ((theByte >= 'A') && (theByte <= 'Z')) ||
				((theByte >= '0') && (theByte <= '9')))
				{
					PutBackChar(theByte);
					break;
				}
				if (theByte < ' ')
				{
					if ((theByte == '\n') && ((TKF_USES_EOL & m_flags) != 0))
					{
						PutBackToken(CUserToken::Create(TK_EOL, "-EOLN-"));
						break;
					}
					continue;
				}
				symbolString[curStrLen++] = theByte;
				symbolString[curStrLen] = '\0';
			}
		}
		return CUndefinedToken::Create(symbolString);
	}
	return CUserToken::Create(lastLookup->GetValue(), symbolString);
}

CToken* CTokenizer::HandleDirective()
{
	int tokenValue = 0;
	CToken* theToken = FetchToken();
	if (theToken->GetType() == TK_EOF)
	{
		return theToken;
	}
	if (theToken->GetType() != TK_IDENTIFIER)
	{
		Error(TKERR_INVALID_DIRECTIVE);
		theToken->Delete();
		SkipToLineEnd();
		return NULL;
	}

	CDirectiveSymbol* curSymbol;
	CTokenizerState* state;
	int theDirective = DirectiveFromName(theToken->GetStringValue());
	theToken->Delete();
	byte theByte;
	switch(theDirective)
	{
	case DIR_INCLUDE:
		if ((m_state != NULL) && (m_state->Skipping()))
		{
			break;
		}
		theToken = GetToken();
		if (theToken->GetType() != TK_STRING)
		{
			Error(TKERR_INCLUDE_FILE_NOTFOUND);
			theToken->Delete();
			SkipToLineEnd();
			break;
		}
		AddParseFile(theToken->GetStringValue());
		theToken->Delete();
		break;
	case DIR_IFDEF:
		if ((m_state != NULL) && (m_state->Skipping()))
		{
			state = CTokenizerHolderState::Create();
			state->SetNext(m_state);
			m_state = state;
			break;
		}
		theToken = GetToken();
		if (theToken->GetType() != TK_IDENTIFIER)
		{
			Error(TKERR_EXPECTED_IDENTIFIER);
			theToken->Delete();
			SkipToLineEnd();
			break;
		}
		state = CTokenizerState::Create(m_defines.FindSymbol(theToken->GetStringValue()) == NULL);
		theToken->Delete();
		state->SetNext(m_state);
		m_state = state;
		break;
	case DIR_IFNDEF:
		if ((m_state != NULL) && (m_state->Skipping()))
		{
			state = CTokenizerHolderState::Create();
			state->SetNext(m_state);
			m_state = state;
			break;
		}
		theToken = GetToken();
		if (theToken->GetType() != TK_IDENTIFIER)
		{
			Error(TKERR_EXPECTED_IDENTIFIER);
			theToken->Delete();
			SkipToLineEnd();
			break;
		}
		state = CTokenizerState::Create(m_defines.FindSymbol(theToken->GetStringValue()) != NULL);
		theToken->Delete();
		state->SetNext(m_state);
		m_state = state;
		break;
	case DIR_ENDIF:
		if (m_state == NULL)
		{
			Error(TKERR_UNMATCHED_DIRECTIVE);
			break;
		}
		state = m_state;
		m_state = state->GetNext();
		state->Delete();
		break;
	case DIR_ELSE:
		if (m_state == NULL)
		{
			Error(TKERR_UNMATCHED_DIRECTIVE);
			break;
		}
		if (!m_state->ProcessElse())
		{
			Error(TKERR_UNMATCHED_DIRECTIVE);
			break;
		}
		break;
	case DIR_DEFINE:
		if ((m_state != NULL) && (m_state->Skipping()))
		{
			break;
		}
		theToken = GetToken();
		if (theToken->GetType() != TK_IDENTIFIER)
		{
			Error(TKERR_EXPECTED_IDENTIFIER);
			theToken->Delete();
			SkipToLineEnd();
			break;
		}
		// blind add - should check first for value changes
		{
			curSymbol = CDirectiveSymbol::Create(theToken->GetStringValue());
			char temp[128];
			int tempsize = 0;
			while(NextChar(theByte))
			{
				if (theByte == '\n')
				{
					break;
				}
				temp[tempsize++] = char(theByte);
			}
			temp[tempsize] = '\0';
			curSymbol->SetValue(temp);
			if (!m_defines.AddSymbol(curSymbol))
			{
				curSymbol->Delete();
			}
		}
		theToken->Delete();
		break;
	case DIR_UNDEFINE:
		if ((m_state != NULL) && (m_state->Skipping()))
		{
			break;
		}
		theToken = GetToken();
		if (theToken->GetType() != TK_IDENTIFIER)
		{
			Error(TKERR_EXPECTED_IDENTIFIER);
			theToken->Delete();
			SkipToLineEnd();
			break;
		}
		m_defines.RemoveSymbol(theToken->GetStringValue());
		theToken->Delete();
		break;
	default:
		Error(TKERR_INVALID_DIRECTIVE);
		SkipToLineEnd();
		break;
	}
	return NULL;
}

COLORREF CTokenizer::ParseRGB()
{
	CToken* theToken = GetToken();
	if (theToken->GetType() != TK_INT)
	{
		Error(TKERR_EXPECTED_INTEGER);
		theToken->Delete();
		return RGB(0, 0, 0);
	}
	int red = theToken->GetIntValue();
	theToken->Delete();
	theToken = GetToken();
	if (theToken->GetType() != TK_INT)
	{
		Error(TKERR_EXPECTED_INTEGER);
		theToken->Delete();
		return RGB(0, 0, 0);
	}
	int green = theToken->GetIntValue();
	theToken->Delete();
	theToken = GetToken();
	if (theToken->GetType() != TK_INT)
	{
		Error(TKERR_EXPECTED_INTEGER);
		theToken->Delete();
		return RGB(0, 0, 0);
	}
	int blue = theToken->GetIntValue();
	theToken->Delete();
	return RGB(red, green, blue);
}

//
// CSymbolLookup
//

CSymbolLookup::CSymbolLookup()
{
}

CSymbolLookup::~CSymbolLookup()
{
}

CSymbolLookup* CSymbolLookup::Create(byte theByte)
{
	CSymbolLookup* curLookup = new CSymbolLookup();
	curLookup->Init(theByte);
	return curLookup;
}

void CSymbolLookup::Delete()
{
	if (m_sibling != NULL)
	{
		m_sibling->Delete();
		m_sibling = NULL;
	}
	if (m_child != NULL)
	{
		m_child->Delete();
		m_child = NULL;
	}
	delete this;
}

void CSymbolLookup::Init(byte theByte)
{
	m_parent = NULL;
	m_child = NULL;
	m_sibling = NULL;
	m_value = -1;
	m_byte = theByte;
}

CSymbolLookup* CSymbolLookup::GetNext()
{
	return m_sibling;
}

void CSymbolLookup::SetNext(CSymbolLookup* next)
{
	m_sibling = next;
}

void CSymbolLookup::SetParent(CSymbolLookup* parent)
{
	m_parent = parent;
}

void CSymbolLookup::SetValue(int value)
{
	m_value = value;
}

int CSymbolLookup::GetValue()
{
	return m_value;
}

byte CSymbolLookup::GetByte()
{
	return m_byte;
}

CSymbolLookup** CSymbolLookup::GetChildAddress()
{
	return &m_child;
}

CSymbolLookup* CSymbolLookup::GetChild()
{
	return m_child;
}

CSymbolLookup* CSymbolLookup::GetParent()
{
	return m_parent;
}

void CToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = cur->GetNext();
		delete(cur);
	}
}

CToken* CToken::m_avail = NULL;

void CToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CCharToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CCharToken*)cur->GetNext();
		delete(cur);
	}
}

CCharToken* CCharToken::m_avail = NULL;

void CCharToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CIntToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CIntToken*)cur->GetNext();
		delete(cur);
	}
}

CIntToken* CIntToken::m_avail = NULL;

void CIntToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CFloatToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CFloatToken*)cur->GetNext();
		delete(cur);
	}
}

CFloatToken* CFloatToken::m_avail = NULL;

void CFloatToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CStringToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CStringToken*)cur->GetNext();
		delete(cur);
	}
}

CStringToken* CStringToken::m_avail = NULL;

void CStringToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CSpaceToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CSpaceToken*)cur->GetNext();
		delete(cur);
	}
}

CSpaceToken* CSpaceToken::m_avail = NULL;

void CSpaceToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CUndefinedToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CUndefinedToken*)cur->GetNext();
		delete(cur);
	}
}

CUndefinedToken* CUndefinedToken::m_avail = NULL;

void CUndefinedToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CUserToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CUserToken*)cur->GetNext();
		delete(cur);
	}
}

CUserToken* CUserToken::m_avail = NULL;

void CUserToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CIdentifierToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CIdentifierToken*)cur->GetNext();
		delete(cur);
	}
}

CIdentifierToken* CIdentifierToken::m_avail = NULL;

void CIdentifierToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}

void CCommentToken::DeleteAll()
{
	while(m_avail != NULL)
	{
		CToken* cur = m_avail;
		m_avail = (CCommentToken*)cur->GetNext();
		delete(cur);
	}
}

CCommentToken* CCommentToken::m_avail = NULL;

void CCommentToken::MakeAvail()
{
	SetNext(m_avail);
	m_avail = this;
}
