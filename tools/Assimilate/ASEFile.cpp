// ASEFile.cpp

#include "Module.h"
#include "Tokenizer.h"
#include "AlertErrHandler.h"
#include "ASEFile.h"

keywordArray_t	CASEFile::s_symbols[] = 
{
	"*",		TK_ASE_ASTERISK,
	"{",		TK_ASE_OBRACE,
	"}",		TK_ASE_CBRACE,
	":",		TK_ASE_COLON,
	NULL,		TK_EOF,
};

keywordArray_t	CASEFile::s_keywords[] = 
{
	"GEOMOBJECT",			TK_GEOMOBJECT,
	"SCENE",				TK_SCENE,
	"MATERIAL_LIST",		TK_MATERIAL_LIST,
	"COMMENT",				TK_ASE_COMMENT,
	"3DSMAX_ASCIIEXPORT",	TK_3DSMAX_ASCIIEXPORT,
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_sceneKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t  CASEFile::s_materialKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_mapKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_objectKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_nodeKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_meshKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_vertexKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_faceKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_faceOptionKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_tvertKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_tfaceKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_animationKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_contorlPosBezierKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_controlRotTCBKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_controlScaleBezierKeywords[] = 
{
	NULL,					TK_EOF,
};

keywordArray_t	CASEFile::s_labelKeywords[] = 
{
	NULL,					TK_EOF,
};

CASEFile::CASEFile()
{
}

CASEFile::~CASEFile()
{
}

CASEFile* CASEFile::Create(LPCTSTR filename)
{
	CASEFile* retval = new CASEFile();
	retval->Init(filename);
	return retval;
}

void CASEFile::Delete()
{
	if (m_file != NULL)
	{
		free(m_file);
		m_file = NULL;
	}
	if (m_comment != NULL)
	{
		free(m_comment);
		m_comment = NULL;
	}
	delete this;
}

void CASEFile::Init(LPCTSTR filename)
{
	if ((filename != NULL) && (strlen(filename) > 0))
	{
		m_file = (char*)malloc(strlen(filename) + 1);
		strcpy(m_file, filename);
	}
	else
	{
		m_file = NULL;
	}
	m_export = 0;
	m_comment = NULL;
}

void CASEFile::Parse()
{
	if (m_file == NULL)
	{
		return;
	}
	CAlertErrHandler errhandler;
	CTokenizer* tokenizer = CTokenizer::Create(TKF_USES_EOL | TKF_NUMERICIDENTIFIERSTART);
	tokenizer->SetErrHandler(&errhandler);
	tokenizer->SetKeywords(s_keywords);
	tokenizer->SetSymbols(s_symbols);
	tokenizer->AddParseFile(m_file, 16 * 1024);
	int tokType = TK_UNDEFINED;
	while(tokType != TK_EOF)
	{
		CToken* curToken = tokenizer->GetToken();
		if (curToken->GetType() == TK_EOF)
		{
			curToken->Delete();
			tokType = TK_EOF;
			break;
		}
		if (curToken->GetType() == TK_EOL)
		{
			curToken->Delete();
			continue;
		}
		if (curToken->GetType() != TK_ASE_ASTERISK)
		{
			tokenizer->Error(TKERR_UNEXPECTED_TOKEN);
			curToken->Delete();
			tokenizer->GetToEndOfLine()->Delete();
			continue;
		}
		curToken->Delete();
		curToken = tokenizer->GetToken();
		tokType = curToken->GetType();
		curToken->Delete();
		switch(tokType)
		{
		case TK_EOF:
			break;
		case TK_GEOMOBJECT:
			ParseGeomObject(tokenizer);
			break;
		case TK_SCENE:
			ParseScene(tokenizer);
			break;
		case TK_MATERIAL_LIST:
			ParseMaterialList(tokenizer);
			break;
		case TK_ASE_COMMENT:
			ParseComment(tokenizer);
			break;
		case TK_3DSMAX_ASCIIEXPORT:
			ParseAsciiExport(tokenizer);
			break;
		default:
			tokenizer->Error(TKERR_UNEXPECTED_TOKEN);
			tokenizer->GetToEndOfLine()->Delete();
			break;
		}
	}
}

void CASEFile::ParseAsciiExport(CTokenizer* tokenizer)
{
	CToken* token = tokenizer->GetToken();
	if (token->GetType() != TK_INT)
	{
		tokenizer->Error(token->GetStringValue(), TKERR_UNEXPECTED_TOKEN);
		token->Delete();
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}
	m_export = token->GetIntValue();
	token->Delete();
}

void CASEFile::ParseComment(CTokenizer* tokenizer)
{
	CToken* token = tokenizer->GetToken();
	if (token->GetType() != TK_STRING)
	{
		tokenizer->Error(token->GetStringValue(), TKERR_UNEXPECTED_TOKEN);
		token->Delete();
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}
	m_comment = (char*)malloc(strlen(token->GetStringValue()) + 1);
	strcpy(m_comment, token->GetStringValue());
	token->Delete();
}

void CASEFile::ParseMaterialList(CTokenizer* tokenizer)
{
}

void CASEFile::ParseScene(CTokenizer* tokenizer)
{
	CToken* token = tokenizer->GetToken(s_sceneKeywords, 0, TKF_USES_EOL);
	if (token->GetType() != TK_ASE_OBRACE)
	{
		tokenizer->Error(token->GetStringValue(), TKERR_UNEXPECTED_TOKEN);
		token->Delete();
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}
	token->Delete();

	bool inScene = true;
	while(inScene)
	{
		token = tokenizer->GetToken(s_sceneKeywords, 0, 0);
		switch(token->GetType())
		{
		case TK_EOF:
			inScene = false;
			tokenizer->Error("End of File", TKERR_UNEXPECTED_TOKEN);
			token->Delete();
			break;
		case TK_ASE_CBRACE:
			token->Delete();
			inScene = false;
			break;
		case TK_EOL:
			token->Delete();
			break;
		case TK_ASE_ASTERISK:
			token->Delete();
			token = tokenizer->GetToken(s_sceneKeywords, 0, 0);
			switch (token->GetType())
			{
			case TK_SCENE_FILENAME:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			case TK_SCENE_FIRSTFRAME:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			case TK_SCENE_LASTFRAME:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			case TK_SCENE_FRAMESPEED:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			case TK_SCENE_TICKSPERFRAME:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			case TK_SCENE_BACKGROUND_STATIC:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			case TK_SCENE_AMBIENT_STATIC:
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			default:
				tokenizer->Error(token->GetStringValue(), TKERR_UNEXPECTED_TOKEN);
				token->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				break;
			}
		default:
			tokenizer->Error(token->GetStringValue(), TKERR_UNEXPECTED_TOKEN);
			token->Delete();
			tokenizer->GetToEndOfLine()->Delete();
		}
	}
}

void CASEFile::ParseGeomObject(CTokenizer* tokenizer)
{
}


