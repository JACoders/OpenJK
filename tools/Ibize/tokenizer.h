// Tokenizer.h
//

#ifndef __TOKENIZER_H
#define __TOKENIZER_H

#pragma warning( disable : 4786 )  // identifier was truncated 

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "ibize_platform.h"

typedef unsigned char byte;
typedef unsigned short word;

#define MAX_STRING_LENGTH		256
#define MAX_IDENTIFIER_LENGTH	128

#define TKF_IGNOREDIRECTIVES			0x00000001		// skip over lines starting with #
#define TKF_USES_EOL					0x00000002		// generate end of line tokens
#define TKF_NODIRECTIVES				0x00000004		// don't treat # in any special way
#define TKF_WANTUNDEFINED				0x00000008		// if token not found in symbols create undefined token
#define TKF_WIDEUNDEFINEDSYMBOLS		0x00000010		// when undefined token encountered, accumulate until space
#define TKF_RAWSYMBOLSONLY				0x00000020
#define TKF_NUMERICIDENTIFIERSTART		0x00000040
#define TKF_IGNOREKEYWORDS				0x00000080
#define TKF_NOCASEKEYWORDS				0x00000100
#define TKF_NOUNDERSCOREINIDENTIFIER	0x00000200
#define TKF_NODASHINIDENTIFIER			0x00000400
#define TKF_COMMENTTOKENS				0x00000800

enum
{
	TKERR_NONE,
	TKERR_UNKNOWN,
	TKERR_BUFFERCREATE,
	TKERR_UNRECOGNIZEDSYMBOL,
	TKERR_DUPLICATESYMBOL,
	TKERR_STRINGLENGTHEXCEEDED,
	TKERR_IDENTIFIERLENGTHEXCEEDED,
	TKERR_EXPECTED_INTEGER,
	TKERR_EXPECTED_IDENTIFIER,
	TKERR_EXPECTED_STRING,
	TKERR_EXPECTED_CHAR,
	TKERR_EXPECTED_FLOAT,
	TKERR_UNEXPECTED_TOKEN,
	TKERR_INVALID_DIRECTIVE,
	TKERR_INCLUDE_FILE_NOTFOUND,
	TKERR_UNMATCHED_DIRECTIVE,
	TKERR_USERERROR,
};

enum
{
	TK_EOF = -1,
	TK_UNDEFINED,
	TK_COMMENT,
	TK_EOL,
	TK_CHAR,
	TK_STRING,
	TK_INT,
	TK_INTEGER = TK_INT,
	TK_FLOAT,
	TK_IDENTIFIER,
	TK_USERDEF,
};

typedef struct
{
	char*		m_keyword;
	int			m_tokenvalue;
} keywordArray_t;

class lessstr
{
public:
	bool operator()(LPCTSTR str1, LPCTSTR str2) const {return (strcmp(str1, str2) < 0);};
};

class CParseStream
{
public:
	CParseStream();
	~CParseStream();
	static CParseStream* Create();
	virtual void Delete();
	virtual bool NextChar(byte& theByte);
	virtual int GetCurLine();
	virtual void GetCurFilename(char** theBuff);
	virtual long GetRemainingSize();

	CParseStream* GetNext();
	void SetNext(CParseStream* next);

	virtual bool IsThisDefinition(void* theDefinition);

protected:
	virtual bool Init();

	CParseStream*		m_next;
};

class CToken
{
public:
	CToken();
	~CToken();
	static CToken* Create();
	virtual void Delete();

	virtual int GetType();
	CToken* GetNext();
	void SetNext(CToken* theToken);
	virtual int GetIntValue();
	virtual LPCTSTR GetStringValue();
	virtual float GetFloatValue();

protected:
	virtual void Init();

	char*			m_string;
	CToken*			m_next;
};

class CCharToken : public CToken
{
public:
	CCharToken();
	~CCharToken();
	static CCharToken* Create(byte theByte);
	virtual void Delete();

	virtual int GetType();

protected:
	virtual void Init(byte theByte);
};

class CStringToken : public CToken
{
public:
	CStringToken();
	~CStringToken();
	static CStringToken* Create(LPCTSTR theString);
	virtual void Delete();

	virtual int GetType();

protected:
	virtual void Init(LPCTSTR theString);
};

class CIntToken : public CToken
{
public:
	CIntToken();
	~CIntToken();
	static CIntToken* Create(long value);
	virtual void Delete();

	virtual int GetType();
	virtual float GetFloatValue();
	virtual int GetIntValue();
	virtual LPCTSTR GetStringValue();

protected:
	virtual void Init(long value);
	
	long			m_value;
};

class CFloatToken : public CToken
{
public:
	CFloatToken();
	~CFloatToken();
	static CFloatToken* Create(float value);
	virtual void Delete();

	virtual int GetType();
	virtual float GetFloatValue();
	virtual LPCTSTR GetStringValue();

protected:
	virtual void Init(float value);

	float			m_value;
};

class CIdentifierToken : public CToken
{
public:
	CIdentifierToken();
	~CIdentifierToken();
	static CIdentifierToken* Create(LPCTSTR name);
	virtual void Delete();

	virtual int GetType();

protected:
	virtual void Init(LPCTSTR name);
};

class CCommentToken : public CToken
{
public:
	CCommentToken();
	~CCommentToken();
	static CCommentToken* Create(LPCTSTR name);
	virtual void Delete();

	virtual int GetType();

protected:
	virtual void Init(LPCTSTR name);
};

class CUserToken : public CToken
{
public:
	CUserToken();
	~CUserToken();
	static CUserToken* Create(int value, LPCTSTR string);
	virtual void Delete();

	virtual int GetType();

protected:
	virtual void Init(int value, LPCTSTR string);

	int				m_value;
};

class CUndefinedToken : public CToken
{
public:
	CUndefinedToken();
	~CUndefinedToken();
	static CUndefinedToken* Create(LPCTSTR string);
	virtual void Delete();

	virtual int GetType();

protected:
	virtual void Init(LPCTSTR string);
};

class CSymbol
{
public:
	CSymbol();
	virtual ~CSymbol();
	static CSymbol* Create(LPCTSTR symbolName);
	virtual void Delete();

	LPCTSTR GetName();

protected:
	virtual void Init(LPCTSTR symbolName);

	char*			m_symbolName;
};

typedef map<LPCTSTR, CSymbol*, lessstr> symbolmap_t;

class CDirectiveSymbol : public CSymbol
{
public:
	CDirectiveSymbol();
	~CDirectiveSymbol();
	static CDirectiveSymbol* Create(LPCTSTR symbolName);
	virtual void Delete();

	void SetValue(LPCTSTR value);
	LPCTSTR GetValue();

protected:
	virtual void Init(LPCTSTR symbolName);

	char*			m_value;
};

class CIntSymbol : public CSymbol
{
public:
	CIntSymbol();
	static CIntSymbol* Create(LPCTSTR symbolName, int value);
	virtual void Delete();

	int GetValue();

protected:
	virtual void Init(LPCTSTR symbolName, int value);

	int				m_value;
};

class CSymbolTable
{
public:
	CSymbolTable();
	~CSymbolTable();
	static CSymbolTable* Create();
	void Delete();

	bool AddSymbol(CSymbol* theSymbol);
	CSymbol* FindSymbol(LPCTSTR symbolName);
	CSymbol* ExtractSymbol(LPCTSTR symbolName);
	void RemoveSymbol(LPCTSTR symbolName);
	void DiscardSymbols();

protected:
	void Init();
	symbolmap_t			m_symbols;
};

class CSymbolLookup
{
public:
	CSymbolLookup();
	~CSymbolLookup();
	static CSymbolLookup* Create(byte theByte);
	virtual void Delete();
	CSymbolLookup* GetNext();
	void SetNext(CSymbolLookup* next);
	void SetParent(CSymbolLookup* parent);
	CSymbolLookup* GetParent();
	CSymbolLookup** GetChildAddress();
	CSymbolLookup* GetChild();
	void SetValue(int value);
	int GetValue();
	byte GetByte();

protected:
	void Init(byte theByte);

	CSymbolLookup*		m_child;
	CSymbolLookup*		m_sibling;
	CSymbolLookup*		m_parent;
	int					m_value;
	byte				m_byte;
};

class CTokenizerState
{
public:
	CTokenizerState();
	~CTokenizerState();
	static CTokenizerState* Create(bool skip);
	virtual void Delete();
	CTokenizerState* GetNext();
	void SetNext(CTokenizerState* next);
	virtual bool ProcessElse();
	bool Skipping();

protected:
	void Init(bool skip);

	bool				m_skip;
	bool				m_elseHit;
	CTokenizerState*	m_next;
};

class CTokenizerHolderState : public CTokenizerState
{
public:
	CTokenizerHolderState();
	~CTokenizerHolderState();
	static CTokenizerHolderState* Create();
	virtual void Delete();
	virtual bool ProcessElse();

protected:
	void Init();
};

typedef void (*LPTokenizerErrorProc)(LPCTSTR errString);

#ifdef USES_MODULES
class CTokenizer : public CModule
#else
class CTokenizer
#endif
{
public:
	CTokenizer();
	~CTokenizer();
	static CTokenizer* Create(UINT dwFlags = 0);
	virtual void Delete();
	virtual void Error(int theError);
	virtual void Error(int theError, LPCTSTR errString);
	virtual void Error(LPCTSTR errString, int theError = TKERR_UNKNOWN);

	CToken* GetToken(UINT onFlags = 0, UINT offFlags = 0);
	CToken* GetToken(keywordArray_t* keywords, UINT onFlags, UINT offFlags);
	void PutBackToken(CToken* theToken, bool commented = false, LPCTSTR addedChars = NULL, bool bIgnoreThisTokenType = false);
	bool RequireToken(int tokenType);
	void ScanUntilToken(int tokenType);
	void SkipToLineEnd();
	CToken* GetToEndOfLine(int tokenType = TK_IDENTIFIER);

	keywordArray_t* SetKeywords(keywordArray_t* theKeywords);
	void SetSymbols(keywordArray_t* theSymbols);
	void SetAdditionalErrors(keywordArray_t* theErrors);
	void SetErrorProc(LPTokenizerErrorProc errorProc);
	void AddParseStream(byte* data, long datasize);
	bool AddParseFile(LPCTSTR filename);
	COLORREF ParseRGB();
	long GetRemainingSize();

	UINT GetFlags();
	void SetFlags(UINT flags);

	void GetCurFilename(char** filename);
	int GetCurLine();

	LPCTSTR LookupToken(int tokenID, keywordArray_t* theTable = NULL);

protected:
	void SetError(int theError, LPCTSTR errString); 
	virtual void Init(UINT dwFlags = 0);
	CToken* FetchToken();
	bool AddDefineSymbol(CDirectiveSymbol* definesymbol);
	bool NextChar(byte& theByte);
	byte Escapement();
	void InsertSymbol(LPCTSTR theSymbol, int theValue);
	void PutBackChar(byte theByte, int curLine = 0, LPCTSTR filename = NULL);
	CToken* TokenFromName(LPCTSTR name);
	CToken* HandleDirective();
	CToken* HandleSlash();
	CToken* HandleString();
	CToken* HandleQuote();
	CToken* HandleIdentifier(byte theByte);
	CToken* HandleNumeric(byte theByte);
	CToken* HandleFloat(bool thesign = false, long value = 0);
	CToken* HandleDecimal(bool thesign = false);
	CToken* HandleSymbol(byte theByte);
	CToken* HandleHex(bool thesize);
	CToken* HandleOctal(bool thesize);
	int DirectiveFromName(LPCTSTR name);

	CParseStream*			m_curParseStream;
	keywordArray_t*			m_keywords;
	keywordArray_t*			m_symbols;
	keywordArray_t*			m_errors;
	CSymbolLookup*			m_symbolLookup;
	CToken*					m_nextToken;
	CSymbolTable			m_defines;
	CTokenizerState*		m_state;
	UINT					m_flags;
	LPTokenizerErrorProc	m_errorProc;

	static keywordArray_t errorMessages[];
	static keywordArray_t directiveKeywords[];
};

class CKeywordTable
{
public:
	CKeywordTable(CTokenizer* tokenizer, keywordArray_t* keywords);
	~CKeywordTable();

protected:
	CTokenizer*			m_tokenizer;
	keywordArray_t*		m_holdKeywords;
};

class CParsePutBack : public CParseStream
{
public:
	CParsePutBack();
	~CParsePutBack();
	static CParsePutBack* Create(byte theByte, int curLine, LPCTSTR filename);
	virtual void Delete();
	virtual bool NextChar(byte& theByte);
	virtual int GetCurLine();
	virtual void GetCurFilename(char** theBuff);
	virtual long GetRemainingSize();

protected:
	virtual void Init(byte theByte, int curLine, LPCTSTR filename);

	byte			m_byte;
	bool			m_consumed;
	int				m_curLine;
	char*			m_curFile;
};

class CParseMemory : public CParseStream
{
public:
	CParseMemory();
	~CParseMemory();
	static CParseMemory* Create(byte* data, long datasize);
	virtual void Delete();
	virtual bool NextChar(byte& theByte);
	virtual int GetCurLine();
	virtual void GetCurFilename(char** theBuff);
	virtual long GetRemainingSize();

protected:
	virtual void Init(byte* data, long datasize);

	byte*			m_data;
	int				m_curLine;
	long			m_curPos;
	long			m_datasize;
	long			m_offset;
};

class CParseBlock : public CParseMemory
{
public:
	CParseBlock();
	~CParseBlock();
	static CParseBlock* Create(byte* data, long datasize);
	virtual void Delete();

protected:
	virtual void Init(byte* data, long datasize);
};

class CParseToken : public CParseStream
{
public:
	CParseToken();
	~CParseToken();
	static CParseToken* Create(CToken* token);
	virtual void Delete();
	virtual bool NextChar(byte& theByte);
	virtual int GetCurLine();
	virtual void GetCurFilename(char** theBuff);
	virtual long GetRemainingSize();

protected:
	virtual void Init(CToken* token);

	byte*			m_data;
	int				m_curLine;
	long			m_curPos;
	long			m_datasize;
	long			m_offset;
};

class CParseDefine : public CParseMemory
{
public:
	CParseDefine();
	~CParseDefine();
	static CParseDefine* Create(CDirectiveSymbol* definesymbol);
	virtual void Delete();
	virtual bool IsThisDefinition(void* theDefinition);

protected:
	virtual void Init(CDirectiveSymbol* definesymbol);

	CDirectiveSymbol*			m_defineSymbol;
};

class CParseFile : public CParseStream
{
public:
	CParseFile();
	~CParseFile();
	static CParseFile* Create();
	static CParseFile* Create(LPCTSTR filename, CTokenizer* tokenizer);
//	static CParseFile* Create(CFile* file, CTokenizer* tokenizer);
	virtual void Delete();
	virtual int GetCurLine();
	virtual void GetCurFilename(char** theBuff);
	virtual long GetRemainingSize();

	virtual bool NextChar(byte& theByte);

protected:
	virtual bool Init();
	virtual bool Init(LPCTSTR filename, CTokenizer* tokenizer);
//	virtual void Init(CFile* file, CTokenizer* tokenizer);
	DWORD GetFileSize();
	void Read(void* buff, UINT buffsize);

//	CFile*			m_file;
	HANDLE			m_fileHandle;
	char*			m_fileName;
	int				m_curLine;
	int				m_curPos;
	byte*			m_buff;
	DWORD			m_curByte;
	DWORD			m_filesize;
	bool			m_ownsFile;
};


#endif//__TOKENIZER_H