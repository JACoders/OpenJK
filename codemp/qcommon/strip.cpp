// this include must remain at the top of every CPP file
#include "../server/server.h"
#include "../game/q_shared.h"
#include "qcommon.h"

//#ifdef _DEBUG
//#include <windows.h>
//#endif

/*
#if !defined(G_LOCAL_H_INC)
	#include "..\game\g_local.h"
#endif
*/

//#include "stdafx.h"

#ifndef _STRIPED_
//	#include "parental.h"
	#include "strip.h"
//	#include "../qcommon/palette.h"
#endif



#ifdef _STRIPED_

	#include "resource.h"		// main symbols
	#include "SP_ConfigFile.h"

cStringPackageED *StringPackage = NULL;

char LanguageList[LANGUAGELIST_MAX][LANGUAGE_LENGTH];
char FlagList[FLAGLIST_MAX][FLAG_LENGTH];

#else

/*static*/ cvar_t	*sp_language;
static cvar_t	*sp_show_strip;

#endif

/*
void LogFile(char *Text, ...)
{
	char	Buffer[16384];
	va_list	argptr;
	FILE	*FH;

	va_start (argptr,Text);
	vsprintf (Buffer,Text,argptr);
	va_end (argptr);

	FH = fopen("c:\\striped.log", "r+");
	if (!FH)
	{
		FH = fopen("c:\\striped.log", "w");
	}
	else
	{
		fseek(FH, 0, SEEK_END);
	}
	fprintf(FH, "%s", Buffer);
	fclose(FH);
}
*/

enum
{
	TK_INVALID = -1,
	TK_VERSION = 0,
	TK_ID,
	TK_REFERENCE,
	TK_DESCRIPTION,
	TK_COUNT,
	TK_FLAGS,
	TK_SP_FLAG1,
	TK_SP_FLAG2,
	TK_SP_FLAG3,
	TK_SP_FLAG4,
	TK_SP_FLAG5,
	TK_SP_FLAG6,
	TK_SP_FLAG7,
	TK_SP_FLAG8,
	TK_SP_FLAG9,
	TK_SP_FLAG_ORIGINAL,
	TK_LEFT_BRACE,
	TK_RIGHT_BRACE,
	TK_INDEX,
	TK_NOTES,
	TK_CONFIG,
	TK_TEXT_LANGUAGE1,
	TK_TEXT_LANGUAGE2,
	TK_TEXT_LANGUAGE3,
	TK_TEXT_LANGUAGE4,
	TK_TEXT_LANGUAGE5,
	TK_TEXT_LANGUAGE6,
	TK_TEXT_LANGUAGE7,
	TK_TEXT_LANGUAGE8,
	TK_TEXT_LANGUAGE9,
	TK_TEXT_LANGUAGE10,
	TK_END
};


char *Tokens[TK_END] = 
{
	"VERSION",
	"ID",
	"REFERENCE",
	"DESCRIPTION",
	"COUNT",
	"FLAGS",
	"SP_FLAG1",
	"SP_FLAG2",
	"SP_FLAG3",
	"SP_FLAG4",
	"SP_FLAG5",
	"SP_FLAG6",
	"SP_FLAG7",
	"SP_FLAG8",
	"SP_FLAG9",
	"SP_FLAG_ORIGINAL",
	"{",
	"}",
	"INDEX",
	"NOTES",
	"CONFIG",
	"TEXT_LANGUAGE1",
	"TEXT_LANGUAGE2", 
	"TEXT_LANGUAGE3", 
	"TEXT_LANGUAGE4", 
	"TEXT_LANGUAGE5", 
	"TEXT_LANGUAGE6", 
	"TEXT_LANGUAGE7", 
	"TEXT_LANGUAGE8", 
	"TEXT_LANGUAGE9", 
	"TEXT_LANGUAGE10" 
};


sFlagPair FlagPairs[] =
{
	{ TK_SP_FLAG1,		SP_FLAG1 },
	{ TK_SP_FLAG2,		SP_FLAG2 },
	{ TK_SP_FLAG3,		SP_FLAG3 },
	{ TK_SP_FLAG4,		SP_FLAG4 },
	{ TK_SP_FLAG5,		SP_FLAG5 },
	{ TK_SP_FLAG6,		SP_FLAG6 },
	{ TK_SP_FLAG7,		SP_FLAG7 },
	{ TK_SP_FLAG8,		SP_FLAG8 },
	{ TK_SP_FLAG9,		SP_FLAG9 },
	{ TK_SP_FLAG_ORIGINAL,		SP_FLAG_ORIGINAL },
	{ TK_INVALID,				0 }
};

sFlagPair LanguagePairs[] =
{
	{ TK_TEXT_LANGUAGE1,	SP_LANGUAGE_ENGLISH },
	{ TK_TEXT_LANGUAGE2,	SP_LANGUAGE_FRENCH },
	{ TK_TEXT_LANGUAGE3,	SP_LANGUAGE_GERMAN },
	{ TK_TEXT_LANGUAGE4,	SP_LANGUAGE_BRITISH },
	{ TK_TEXT_LANGUAGE5,	SP_LANGUAGE_KOREAN },
	{ TK_TEXT_LANGUAGE6,	SP_LANGUAGE_TAIWANESE },
	{ TK_TEXT_LANGUAGE7,	SP_LANGUAGE_ITALIAN },
	{ TK_TEXT_LANGUAGE8,	SP_LANGUAGE_SPANISH },
	{ TK_TEXT_LANGUAGE9,	SP_LANGUAGE_JAPANESE },
	{ TK_TEXT_LANGUAGE10,	SP_LANGUAGE_10},
	{ TK_INVALID,		0 }
};

/************************************************************************************************
 * FindToken
 *
 * inputs:
 *	token string
 *	flag indicating if token string is partial or not
 *
 * return:
 *	token enum
 *
 ************************************************************************************************/
int FindToken(char *token, bool whole)
{
	int token_value;
	int	i;

	for(token_value = 0; token_value != TK_END; token_value++)
	{
		if (whole)
		{
			if (Q_stricmp(token, Tokens[token_value]) == 0)
			{
				return token_value;
			}
		}
		else
		{
			if (Q_stricmpn(token, Tokens[token_value], strlen(Tokens[token_value])) == 0)
			{
				i = strlen(Tokens[token_value]);
				while(token[i] == ' ')
				{
					i++;
				}
				memmove(token, &token[i], strlen(token)-i+1);

				return token_value;
			}
		}
	}

	return TK_INVALID;
}

/************************************************************************************************
 * ReadData
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
bool ReadData(char *&Data, int &Size, char *Result, int Result_Size)
{
	char *pos;

	Result[0] = 0;

	if (Size <= 0)
	{
		return false;
	}
	pos = Result;
	do
	{
		*pos = *Data;
		pos++;
		Data++;
		Size--;
		Result_Size--;
	} while(Size > 0 && Result_Size > 0 && *(Data-1) != '\n');

	*pos = 0;
  
	return true;
}

/************************************************************************************************
 * GetLine
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
void GetLine(char *&Data, int &Size, int &token, char *&data)
{
	static char		save_data[8192];
	char			temp_data[8192];
	char			*test_token, *pos;

	save_data[0] = 0;
	token = TK_INVALID;
	data = save_data;

	if (!ReadData(Data, Size, temp_data, sizeof(temp_data)))
	{
		return;
	}

//	strcpy(temp_data, "   DATA \"test of the data\ntest test\ndfa dfd");
//	strcpy(temp_data, "   DATA");

	pos = temp_data;
	while((*pos) && strchr(" \n\r", *pos))
	{	// remove white space
		pos++;
	}
	test_token = pos;

	while((*pos) && !strchr(" \n\r", *pos))
	{	// scan until end of white space
		pos++;
	}

	if ((*pos))
	{
		*pos = 0;
		pos++;
	}
	token = FindToken(test_token, true);
	
	while((*pos) && strchr(" \n\r", *pos))
	{	// remove white space
		pos++;
	}

	if ((*pos) == '\"')
	{
		pos++;
		test_token = save_data;
		memset(save_data, 0, sizeof(save_data));
		
		while(((*pos) != '\"' || !strchr("\n\r", (*(pos+1)))) && (*pos))
		{
			if ((*pos) == '\\' && (*(pos+1)) == 'n')
			{
#ifdef _STRIPED_
				*test_token = '\r';
				test_token++;
#endif
				*test_token = '\n';
				test_token++;
				pos+=2;
				continue;
			}

			*test_token = *pos;
			test_token++;
			pos++;
		}

		if ((*pos) == '\"')
		{
			*pos = 0;
		}
	}
	else
	{
		test_token = pos;
		while((*pos) && !strchr("\n\r", *pos))
		{	// scan until end of white space
			pos++;
		}
		*pos = 0;

		strcpy(save_data, test_token);
	}
}


#ifdef _STRIPED_

/************************************************************************************************
 * SaveString
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
void SaveString(FILE *FH, char *data)
{
	fputc('\"', FH);

	if (data)
	{
		for(;*data;data++)
		{
			if ((*data) == '\r')
			{
			}
			else if ((*data) == '\n')
			{
				fputc('\\', FH);
				fputc('n', FH);
			}
			else
			{
				fputc(*data, FH);
			}
		}
	}

	fputc('\"', FH);
}

#endif


//======================================================================


/************************************************************************************************
 * cCriteria
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
cCriteria::cCriteria(int initWhichLanguage)
{
	WhichLanguage = initWhichLanguage;
}


#ifdef _STRIPED_

/************************************************************************************************
 * cCriteriaED
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
cCriteriaED::cCriteriaED(int initWhichLanguage, cStringPackageED *initMerge)
:cCriteria(initWhichLanguage)
{
	Merge = initMerge;
}

#endif


//======================================================================

/************************************************************************************************
 * cStrings
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
cStrings::cStrings(unsigned int initFlags, char *initReference)
{
	Flags = initFlags;
	Reference = NULL;

	SetReference(initReference);
}

/************************************************************************************************
 * ~cStrings
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
cStrings::~cStrings(void)
{
	Clear();
}

/************************************************************************************************
 * Clear
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
void cStrings::Clear(void)
{
	Flags = 0;

	if (Reference)
	{
		delete Reference;
		Reference = NULL;
	}
}

/************************************************************************************************
 * SetFlags
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
void cStrings::SetFlags(unsigned int newFlags) 
{ 
	Flags = newFlags; 
}


void cStrings::SetReference(char *newReference)
{
	if (Reference)
	{
		delete Reference;
		Reference = NULL;
	}

	if (!newReference || !newReference[0])
	{
		return;
	}

	Reference = new char[strlen(newReference)+1];
	strcpy(Reference, newReference);
}

bool cStrings::UnderstandToken(int token, char *data)
{
	sFlagPair		*FlagPair;

	switch(token)
	{
		case TK_FLAGS:
			while(token != TK_INVALID)
			{
				token = FindToken(data, false);
				for(FlagPair = FlagPairs; FlagPair->Name != TK_INVALID; FlagPair++)
				{
					if (FlagPair->Name == token)
					{
						Flags |= FlagPair->Value;
						break;
					}
				}
			}
			return true;

		case TK_REFERENCE:
			SetReference(data);
			return true;

		case TK_RIGHT_BRACE:
			return false;
	}

	if (token == TK_INVALID)
	{
		return false;
	}

	return true;
}

bool cStrings::SubSave(FILE *FH)
{
	sFlagPair	*FlagPair;

	if (Flags)
	{
		fprintf(FH, "   %s", Tokens[TK_FLAGS]);
		for(FlagPair = FlagPairs; FlagPair->Name != TK_INVALID; FlagPair++)
		{
			if (Flags & FlagPair->Value)
			{
				fprintf(FH, " %s", Tokens[FlagPair->Name]);
			}
		}
		fprintf(FH,"\n");
	}

	if (Reference)
	{
		fprintf(FH, "   %s %s\n", Tokens[TK_REFERENCE], Reference);
	}

	return true;
}

bool cStrings::Save(FILE *FH)
{
	fprintf(FH,"%s\n", Tokens[TK_LEFT_BRACE]);

	SubSave(FH);

	fprintf(FH,"%s\n", Tokens[TK_RIGHT_BRACE]);

	return true;
}

bool cStrings::Load(char *&Data, int &Size)
{
	int				token;
	char			*data;

	Clear();

	GetLine(Data, Size, token, data);
	if (token != TK_LEFT_BRACE)
	{
		return false;
	}

	GetLine(Data, Size, token, data);
	while (UnderstandToken(token, data))
	{
		GetLine(Data, Size, token, data);
	}

	if (token != TK_RIGHT_BRACE)
	{
		return false;
	}

	return true;
}







#ifdef _STRIPED_

cStringsED::cStringsED(unsigned int initFlags, char *initReference, char *initNotes)
:cStrings(initFlags, initReference)
{
	int i;

	Used = false;
	Notes = NULL;

	for(i=0;i<MAX_LANGUAGES;i++)
	{
		Text[i] = NULL;
	}

	SetNotes(initNotes);
}

cStringsED::~cStringsED()
{
	Clear();
}

void cStringsED::Clear(void)
{
	int i;

	cStrings::Clear();

	Used = false;

	for(i=0;i<MAX_LANGUAGES;i++)
	{
		if (Text[i])
		{
			delete Text[i];
			Text[i] = NULL;
		}
	}

	if (Notes)
	{
		delete Notes;
		Notes = NULL;
	}
}

void cStringsED::SetText(int index, char *newText)
{
	if (Text[index])
	{
		delete Text[index];
		Text[index] = NULL;
	}

	if (!newText || !newText[0])
	{
		return;
	}

	Text[index] = new char[strlen(newText)+1];
	strcpy(Text[index], newText);
}

void cStringsED::SetNotes(char *newNotes)
{
	if (Notes)
	{
		delete Notes;
		Notes = NULL;
	}

	if (!newNotes || !newNotes[0])
	{
		return;
	}

	Notes = new char[strlen(newNotes)+1];
	strcpy(Notes, newNotes);
}


bool cStringsED::UnderstandToken(int token, char *data, cCriteria &Criteria)
{
	sFlagPair		*LanguagePair;

	switch(token)
	{
		case TK_NOTES:
			SetNotes(data);
			return true;

		default:
			for(LanguagePair = LanguagePairs; LanguagePair->Name != TK_INVALID; LanguagePair++)
			{
				if (LanguagePair->Name == token)
				{
					SetText(LanguagePair->Value, data);
					return true;
				}
			}

			return cStrings::UnderstandToken(token, data, Criteria);
	}
}

bool cStringsED::SubSave(FILE *FH, cCriteria &Criteria)
{
	int			i;
	sFlagPair	*LanguagePair;

	cStrings::SubSave(FH, Criteria);

	if (Notes)
	{
		fprintf(FH, "   %s ", Tokens[TK_NOTES]);
		SaveString(FH, Notes);
		fprintf(FH, "\n");
	}

	for(i=0;i<MAX_LANGUAGES;i++)
	{
		if (Criteria.WhichLanguage != TEXT_ALL && Criteria.WhichLanguage != i && i != TEXT_LANGUAGE1)
		{	// we aren't saving everything, and this isn't the matching text or english
			continue;
		}

		if (Text[i])
		{
			for(LanguagePair = LanguagePairs; LanguagePair->Name != TK_INVALID; LanguagePair++)
			{
				if (i == (int)LanguagePair->Value)
				{
					fprintf(FH, "   %s ",Tokens[LanguagePair->Name]);
					SaveString(FH, Text[i]);
					fprintf(FH, "\n");
				}
			}
		}
	}

	return true;
}

bool cStringsED::Load(char *&Data, int &Size, cCriteria &Criteria)
{
	if (cStrings::Load(Data, Size, Criteria))
	{
		Used = true;
		return true;
	}

	return false;
}

#endif



#ifndef _STRIPED_

cStringsSingle::cStringsSingle(unsigned int initFlags, char *initReference)
:cStrings(initFlags, initReference)
{
	Text = NULL;
}

cStringsSingle::~cStringsSingle()
{
	Clear();
}

void cStringsSingle::Clear(void)
{
	cStrings::Clear();

	if (Text)
	{
		delete Text;
		Text = NULL;
	}
}

void cStringsSingle::SetText(const char *newText)
{
	int		length;
	char	*Dest;

	if (Text)
	{
		delete Text;
		Text = NULL;
	}

	if (!newText || !newText[0])
	{
		return;
	}

	length = strlen(newText)+1;

#ifndef _STRIPED_
	// Following is for TESTING for SOF.
	if(sp_show_strip->value)
	{
		Dest = Text = new char[length + 6];
		strcpy(Dest,"SP:");
		Dest += strlen(Dest);
	}
	else
#endif
	{
		Dest = Text = new char[length];
	}
	strcpy(Dest, newText);
}
	
// fix problems caused by fucking morons entering clever "rich" chars in to new text files *after* the auto-stripper
//	removed them all in the first place...
//
// ONLY DO THIS FOR EUROPEAN LANGUAGES, OR IT BREAKS ASIAN STRINGS!!!!!!!!!!!!!!!!!!!!!
//
static void FixIllegalChars(char *psText)
{
	char *p;

//	strXLS_Speech.Replace(va("%c",0x92),va("%c",0x27));	// "'"
	while ((p=strchr(psText,0x92))!=NULL)  // "rich" (and illegal) apostrophe
	{
		*p = 0x27;
	}

//	strXLS_Speech.Replace(va("%c",0x93),"\"");			// smart quotes -> '"'
	while ((p=strchr(psText,0x93))!=NULL)  // "rich" (and illegal) apostrophe
	{
		*p = '"';
	}

//	strXLS_Speech.Replace(va("%c",0x94),"\"");			// smart quotes -> '"'
	while ((p=strchr(psText,0x94))!=NULL)  // "rich" (and illegal) apostrophe
	{
		*p = '"';
	}

//	strXLS_Speech.Replace(va("%c",0x0B),".");			// full stop
	while ((p=strchr(psText,0x0B))!=NULL)  // "rich" (and illegal) apostrophe
	{
		*p = '.';
	}

//	strXLS_Speech.Replace(va("%c",0x85),"...");			// "..."-char ->  3-char "..."
	while ((p=strchr(psText,0x85))!=NULL)  // "rich" (and illegal) apostrophe
	{
		*p = '.';	// can't do in-string replace of "." with "...", so just forget it
	}

//	strXLS_Speech.Replace(va("%c",0x91),va("%c",0x27));	// "'"
	while ((p=strchr(psText,0x91))!=NULL)  // "rich" (and illegal) apostrophe
	{
		*p = 0x27;
	}

//	strXLS_Speech.Replace(va("%c",0x96),va("%c",0x2D));	// "-"
	while ((p=strchr(psText,0x96))!=NULL)
	{
		*p = 0x2D;
	}

//	strXLS_Speech.Replace(va("%c",0x97),va("%c",0x2D));	// "-"
	while ((p=strchr(psText,0x97))!=NULL)
	{
		*p = 0x2D;
	}

	// StripEd and our print code don't support tabs...
	//
	while ((p=strchr(psText,0x09))!=NULL)
	{
		*p = ' ';
	}	
}

bool cStringsSingle::UnderstandToken(int token, char *data)
{
	sFlagPair		*LanguagePair;

//	switch(token)
//	{
//		default:

			for(LanguagePair = LanguagePairs; LanguagePair->Name != TK_INVALID; LanguagePair++)
			{
				if (LanguagePair->Name == TK_TEXT_LANGUAGE1 && token == TK_TEXT_LANGUAGE1 && !Text)
				{	// default to english in case there is no foreign
					if (LanguagePair->Name == TK_TEXT_LANGUAGE1 ||
						LanguagePair->Name == TK_TEXT_LANGUAGE2 ||
						LanguagePair->Name == TK_TEXT_LANGUAGE3 ||
						LanguagePair->Name == TK_TEXT_LANGUAGE8
						)
					{
						FixIllegalChars(data);
					}
					SetText(data);
					return true;
				}
				else if (LanguagePair->Name == token && LanguagePair->Value == (int)sp_language->value)
				{
					if (LanguagePair->Name == TK_TEXT_LANGUAGE1 ||
						LanguagePair->Name == TK_TEXT_LANGUAGE2 ||
						LanguagePair->Name == TK_TEXT_LANGUAGE3 ||
						LanguagePair->Name == TK_TEXT_LANGUAGE8
						)
					{
						FixIllegalChars(data);
					}
					SetText(data);
					return true;
				}
			}

			return cStrings::UnderstandToken(token, data);
//	}
}

#endif








cStringPackage::cStringPackage(const char *in, unsigned char initID, char *initDescription, char *initReference)
{
	ID = initID;
	Registration = 0;
	name = in;
	Reference = NULL;

	SetReference(initReference);
}

cStringPackage::~cStringPackage(void)
{
	if (Reference)
	{
		delete Reference;
		Reference = NULL;
	}
}

void cStringPackage::SetReference(char *newReference)
{
	if (Reference)
	{
		delete Reference;
		Reference = NULL;
	}

	if (!newReference || !newReference[0])
	{
		return;
	}

	Reference = new char[strlen(newReference)+1];
	strcpy(Reference, newReference);
}

#ifndef _STRIPED_

bool cStringPackage::UnderstandToken(char *&Data, int &Size, int token, char *data )
{
	cCriteria FullCriteria;

	switch(token)
	{
		case TK_ID:
			ID = (unsigned char)atol(data);
			return true;

		case TK_CONFIG:
#ifdef _STRIPED_
			ConfigFile.Load(data,FullCriteria);
#endif
			return true;

		case TK_REFERENCE:
#ifdef _STRIPED_
			if (Reference)
			{
				char temp[1024];

				sprintf(temp, "Please get RJ about this error!  Do NOT save or do anything!  OldRefeence: %s, New: %s", Reference, data);
				MessageBox(NULL, temp, "RJ Error!", MB_OK);
			}
#endif
			SetReference(data);
			return true;
	}

	if (token == TK_INVALID)
	{
		return false;
	}

	return true;
}

#endif

bool cStringPackage::SubSave(FILE *FH )
{
	fprintf(FH,"%s %d\n",Tokens[TK_ID], ID);

	if (Reference)
	{
		fprintf(FH,"%s %s\n", Tokens[TK_REFERENCE], Reference);
	}

	return true;
}

#ifdef _STRIPED_

bool cStringPackage::Save(char *FileName)
{
	FILE	*FH;

	FH = fopen(FileName,"w");
	if (!FH)
	{
		return false;
	}

	fprintf(FH,"%s %d\n",Tokens[TK_VERSION], STRIP_VERSION);
	fprintf(FH,"%s %s\n",Tokens[TK_CONFIG], ConfigFile.fileName);

	SubSave(FH);

	fclose(FH);

	return true;
}
#endif

bool cStringPackage::Load(char *FileName)
{
	FILE	*FH;
	int		Size;
	char	*buffer;

	FH = fopen(FileName,"rb");
	if (!FH)
	{
		return false;
	}

	fseek(FH, 0, SEEK_END);
	Size = ftell(FH);
	fseek(FH, 0, SEEK_SET);

	buffer = new char[Size];
	fread(buffer, 1, Size, FH);
	fclose(FH);

	Load(buffer, Size);

	delete buffer;

	return true;
}

bool cStringPackage::Load(char *Data, int &Size)
{
	char	*token_data;
	int		token;

	GetLine(Data, Size, token, token_data);
	if (token != TK_VERSION || atol(token_data) != STRIP_VERSION)
	{
		return false;
	}

	GetLine(Data, Size, token, token_data);
	while (UnderstandToken(Data, Size, token, token_data ))
	{
		GetLine(Data, Size, token, token_data);
	}

	return true;
}



#ifdef _STRIPED_


cStringPackageED::cStringPackageED(unsigned char initID, char *initDescription, char *initReference)
:cStringPackage("", initID, initReference)
{
	Description = NULL;

	SetDescription(initDescription);
}

cStringPackageED::~cStringPackageED(void)
{
	if (Description)
	{
		delete Description;
		Description = NULL;
	}
}

void cStringPackageED::SetDescription(char *newDescription)
{
	if (Description)
	{
		delete Description;
		Description = NULL;
	}

	if (!newDescription || !newDescription[0])
	{
		return;
	}

	Description = new char[strlen(newDescription)+1];
	strcpy(Description, newDescription);
}

cStringsED *cStringPackageED::FindString(int &index)
{
	if (index == -1)
	{
		for(index=0;index<MAX_STRINGS;index++)
		{
			if (!Strings[index].GetUsed())
			{
				return &Strings[index];
			}
		}

		return NULL;
	}
	else
	{
		return &Strings[index];
	}
}

void cStringPackageED::ClearString(int index)
{
	Strings[index].Clear();
}

bool cStringPackageED::UnderstandToken(char *&Data, int &Size, int token, char *data )
{
	int	count, i, pos;

	switch(token)
	{
		case TK_DESCRIPTION:
			SetDescription(data);
			return true;

		case TK_COUNT:
			count = atol(data);

			for(i=0;i<count;i++)
			{
				GetLine(Data, Size, token, data);
				if (token != TK_INDEX)
				{
					return false;
				}
				pos = atol(data);
				if (!Strings[pos].Load(Data, Size ))
				{
					return false;
				}
			}
			return true;

		default:
			return cStringPackage::UnderstandToken(Data, Size, token, data );
	}
}

bool cStringPackageED::SubSave(FILE *FH, cCriteria )
{
	int		i, count;

	cStringPackage::SubSave(FH);

	if (Description)
	{
		fprintf(FH,"%s \"%s\"\n", Tokens[TK_DESCRIPTION], Description);
	}

	for(i=count=0;i<MAX_STRINGS;i++)
	{
		if (Strings[i].GetUsed())
		{
			count++;
		}
	}
	fprintf(FH, "%s %d\n",Tokens[TK_COUNT], count);

	for(i=count=0;i<MAX_STRINGS;i++)
	{
		if (Strings[i].GetUsed())
		{
			fprintf(FH, "%s %d\n", Tokens[TK_INDEX], i);
			Strings[i].Save(FH );
		}
	}

	return true;
}

bool cStringPackageED::GenerateCHeader(char *FileName)
{
	FILE	*FH;
	char	FilePath[MAX_PATH], *FileNamePos, *Test;
	int		i;
	bool	NeedComma = false;;

	if (!Reference)
	{
		return false;
	}

	FH = fopen(FileName, "w");
	if (!FH)
	{
		return false;
	}

	GetFullPathName(FileName, sizeof(FilePath), FilePath, &FileNamePos);

	for(Test = FilePath; (*Test); Test++)
	{
		if (strchr(" .", *Test))
		{
			*Test = '_';
		}
	}
	
	fprintf(FH, "#ifndef __%s\n",FileNamePos);
	fprintf(FH, "#define __%s\n",FileNamePos);
	fprintf(FH, "\n\n");
	if (Description && Description[0])
	{
		fprintf(FH, "/********************************************************************************\n");
		fprintf(FH, "\n");
		fprintf(FH, "%s\n", Description);
		fprintf(FH, "\n");
		fprintf(FH, "********************************************************************************/\n");
		fprintf(FH, "\n\n");
	}

	fprintf(FH, "#define PACKAGE_%s\t\t0x%02x\n", Reference, (unsigned int)ID);
	fprintf(FH, "\n");

	fprintf(FH, "enum\n");
	fprintf(FH, "{\n");
	for(i=0;i<MAX_STRINGS;i++)
	{
		if (Strings[i].GetUsed())
		{
			Test = Strings[i].GetReference();
			if (Test && Test[0])
			{
				if (NeedComma)
				{
					fprintf(FH, ",\n");
				}
				else
				{
					NeedComma = true;
				}
				fprintf(FH, "\t%s_%s = 0x%02x%02x", Reference, Test, (unsigned int)ID, i);
			}
		}
	}
	if (NeedComma)
	{
		fprintf(FH, "\n");
	}
	fprintf(FH, "};\n");

	fprintf(FH, "\n\n");
	fprintf(FH, "#endif // __%s\n", FileNamePos);

	fclose(FH);

	return true;
}

bool cStringPackageED::GenerateDSHeader(char *FileName)
{
	FILE	*FH;
	char	FilePath[MAX_PATH], *FileNamePos, *Test;
	int		i;

	if (!Reference)
	{
		return false;
	}

	FH = fopen(FileName, "w");
	if (!FH)
	{
		return false;
	}

	GetFullPathName(FileName, sizeof(FilePath), FilePath, &FileNamePos);

	for(Test = FilePath; (*Test); Test++)
	{
		if (strchr(" .", *Test))
		{
			*Test = '_';
		}
	}
	
	fprintf(FH, "// __%s\n",FileNamePos);
	fprintf(FH, "//\n");
	fprintf(FH, "\n\n");
	if (Description && Description[0])
	{
		fprintf(FH, "/********************************************************************************\n");
		fprintf(FH, "\n");
		fprintf(FH, "%s\n", Description);
		fprintf(FH, "\n");
		fprintf(FH, "********************************************************************************/\n");
		fprintf(FH, "\n\n");
	}

	fprintf(FH, "#define PACKAGE_%s\t\t0x%02x\n", Reference, (unsigned int)ID);
	fprintf(FH, "\n");

	for(i=0;i<MAX_STRINGS;i++)
	{
		if (Strings[i].GetUsed())
		{
			Test = Strings[i].GetReference();
			if (Test && Test[0])
			{
				fprintf(FH, "#define %s_%s\t\t\t0x%02x%02x\n", Reference, Test, (unsigned int)ID, i);
			}
		}
	}

	fprintf(FH, "\n\n");
	fprintf(FH, "// __%s\n", FileNamePos);

	fclose(FH);

	return true;
}

#endif










#ifndef _STRIPED_


cStringPackageSingle::cStringPackageSingle(const char *in, unsigned char initID, char *initReference)
:cStringPackage(in, initID, initReference)
{
}

cStringPackageSingle::~cStringPackageSingle(void)
{
	ReferenceTable.clear();
}

cStringsSingle *cStringPackageSingle::FindString(char *ReferenceLookup)
{
	int	index;

	index = FindStringID(ReferenceLookup);
	if (index == -1)
	{
		return NULL;
	}

	return FindString(index & SP_STRING);
}

int cStringPackageSingle::FindStringID(const char *ReferenceLookup)
{
	map<string, int>::iterator	i;
	int							size;

	if (!Reference)
	{
		return -1;
	}

	size = strlen(Reference);
	if (strlen(ReferenceLookup) < size+2)
	{
		return -1;
	}

	if (strncmp(ReferenceLookup, Reference, size))
	{
		return -1;
	}

	i = ReferenceTable.find(string(ReferenceLookup + size + 1));
	if (i != ReferenceTable.end())
	{
		return (*i).second;
	}


//#ifdef _DEBUG
//	// findmeste
//	for (map<string, int>::iterator it = ReferenceTable.begin(); it != ReferenceTable.end(); ++it)
//	{
//		OutputDebugString(va("%s\n",(*it).first.c_str()));
//	}
//#endif

	return -1;
}

bool cStringPackageSingle::UnderstandToken(char *&Data, int &Size, int token, char *data )
{
	int		count, i, pos;
	char	*ReferenceLookup;

	switch(token)
	{
		case TK_COUNT:
			count = atol(data);

			for(i=0;i<count;i++)
			{
				GetLine(Data, Size, token, data);
				if (token != TK_INDEX)
				{
					return false;
				}
				pos = atol(data);
				if (!Strings[pos].Load(Data, Size))
				{
					return false;
				}
				ReferenceLookup = Strings[pos].GetReference();
				if (ReferenceLookup)
				{
					ReferenceTable[string(ReferenceLookup)] = pos;
				}
			}
			return true;

		default:
			return cStringPackage::UnderstandToken(Data, Size, token, data);
	}
}

#endif








#ifndef _STRIPED_

// A map of loaded string packages
map<string, cStringPackageSingle *>		SP_ListByName;
map<byte, cStringPackageSingle *>		SP_ListByID;


// Registration
cStringPackageSingle *SP_Register(const char *inPackage, unsigned char Registration)
{
	char											*buffer;
	char											Package[MAX_QPATH];
	int												size;
	cStringPackageSingle							*new_sp;
	map<string, cStringPackageSingle *>::iterator	i;


	assert(SP_ListByName.size() == SP_ListByID.size());

	Q_strncpyz(Package, inPackage, MAX_QPATH);
	Q_strupr(Package);

	i = SP_ListByName.find(Package);
	if (i != SP_ListByName.end())
	{
		new_sp = (*i).second;
	}
	else
	{
		size = FS_ReadFile(va("strip/%s.sp", Package), (void **)&buffer);
		if (size == -1)
		{
			if ( Registration & SP_REGISTER_REQUIRED )
			{
				Com_Error(ERR_FATAL, "Could not open string package '%s'", Package);
			}
			return NULL;
		}
		
		// Create the new string package
		new_sp = new cStringPackageSingle(Package);
		new_sp->Load(buffer, size );
		FS_FreeFile(buffer);
		
		if (Registration & SP_REGISTER_CLIENT)
		{
			Com_DPrintf(S_COLOR_YELLOW "SP_Register: Registered client string package '%s' with ID %02x\n", Package, (int)new_sp->GetID());
		}
		else
		{
			Com_DPrintf(S_COLOR_YELLOW "SP_Register: Registered string package '%s' with ID %02x\n", Package, (int)new_sp->GetID());
		}
		
		// Insert into the name vs package map
		SP_ListByName[Package] = new_sp;
		// Insert into the id vs package map
		SP_ListByID[new_sp->GetID()] = new_sp;
	}
	// Or in the new registration data
	new_sp->Register(Registration);

	return new_sp;
}

// Update configstrings array on clients and server
qboolean SP_RegisterServer(const char *Package)
{
	cStringPackageSingle	*sp;

	sp = SP_Register(Package, SP_REGISTER_SERVER);
	if (sp)
	{
		SV_AddConfigstring(Package,CS_STRING_PACKAGES,MAX_STRING_PACKAGES);
		return qtrue;
	}

	return qfalse;
}

// Unload all packages with the relevant registration bits
void SP_Unload(unsigned char Registration)
{
	map<string, cStringPackageSingle *>::iterator	i, next;
	map<byte, cStringPackageSingle *>::iterator		id;

	assert(SP_ListByName.size() == SP_ListByID.size());

	for(i = SP_ListByName.begin(); i != SP_ListByName.end(); i = next)
	{
		next = i;
		next++;

		if ((*i).second->UnRegister(Registration))
		{
			Com_DPrintf(S_COLOR_YELLOW "SP_UnRegister: Package '%s' with ID %02x\n", (*i).first.c_str(), (int)(*i).second->GetID());

			id = SP_ListByID.find((*i).second->GetID());
			SP_ListByID.erase(id);
			delete (*i).second;
			SP_ListByName.erase(i);
		}
	}

}

// Direct string functions

int SP_GetStringID(const char *inReference)
{
	map<unsigned char,cStringPackageSingle *>::iterator	i;
	int													ID;
	char Reference[MAX_QPATH];
	Q_strncpyz(Reference, inReference, MAX_QPATH);
	Q_strupr(Reference);

	for(i = SP_ListByID.begin(); i != SP_ListByID.end(); i++)
	{
		ID = (*i).second->FindStringID(Reference);
		if (ID >= 0)
		{
			ID |= ((int)(*i).first) << 8;
			return ID;
		}
	}
	return -1;
}

/************************************************************************************************
 * SP_GetString
 *
 * inputs:
 *	ID of the string package
 *
 * return:
 *	pointer to desired String Package
 *
 ************************************************************************************************/
cStringsSingle *SP_GetString(unsigned short ID)
{
	cStringPackageSingle								*sp;
	cStringsSingle										*string;
	map<unsigned char,cStringPackageSingle *>::iterator	i;

	i = SP_ListByID.find(SP_GET_PACKAGE(ID));
	if (i == SP_ListByID.end())
	{
		Com_Error(ERR_DROP, "String package not registered for ID %04x", ID);

		return NULL;
	}

	sp = (*i).second;
	string = sp->FindString(ID & SP_STRING);

	if (!string)
	{
		Com_Error(ERR_DROP, "String ID %04x not defined\n", ID);
	}

	return string;
}

cStringsSingle *SP_GetString(const char *Reference)
{
	int	index;

	index = SP_GetStringID(Reference);
	if (index == -1)
	{
		return NULL;
	}

	return SP_GetString(index);
}


const char *SP_GetStringText(unsigned short ID)
{
	cStringsSingle			*string;
	char					*value;

	string = SP_GetString(ID);

	value = string->GetText();
	if (!value)
	{
		value = "";
	}

	return value;
}

const char *SP_GetStringTextString(const char *Reference)
{
	int	index;

	index = SP_GetStringID(Reference);
	if (index == -1)
	{
		return "";
	}

	return SP_GetStringText(index);
}


static void SP_UpdateLanguage(void)
{
	map<unsigned char, cStringPackageSingle *>::iterator	it;
	list<cStringPackageID>									sps;
	list<cStringPackageID>::iterator						spit;

	// Grab all SP ids
	for(it = SP_ListByID.begin(); it != SP_ListByID.end(); it++)
	{
		sps.push_back(cStringPackageID((*it).second->GetName(), (*it).second->GetRegistration()));
	}
	// Clear out all pointers
	SP_Unload(SP_REGISTER_CLIENT | SP_REGISTER_SERVER | SP_REGISTER_MENU | SP_REGISTER_REQUIRED);

	// Reinitialise with new language
	for(spit = sps.begin(); spit != sps.end(); spit++)
	{
		SP_Register((*spit).GetName(), (*spit).GetReg());	
	}
	sps.clear();
}

void SP_Init(void)
{
	sp_language = Cvar_Get("sp_language", va("%d", SP_LANGUAGE_ENGLISH), CVAR_ARCHIVE | CVAR_NORESTART);
	sp_show_strip = Cvar_Get ("sp_show_strip", "0", 0);

	SP_UpdateLanguage();
	sp_language->modified = qfalse;

	// Register_StringPackets...
	//	
	SP_Register("con_text", SP_REGISTER_REQUIRED);	//reference is CON_TEXT
	SP_Register("mp_ingame",SP_REGISTER_REQUIRED);	//reference is INGAMETEXT
	SP_Register("mp_svgame",SP_REGISTER_REQUIRED);	//reference is SVINGAME
	SP_Register("sp_ingame",SP_REGISTER_REQUIRED);	//reference is INGAME	, needed for item pickups
	SP_Register("keynames", 0/*SP_REGISTER_REQUIRED*/);	//reference is KEYNAMES
}

// called in Com_Frame, so don't take up any time! (can also be called during dedicated)
//
void SP_CheckForLanguageUpdates(void)
{
	if (sp_language && sp_language->modified)
	{
		SP_Init();	// force language package to reload
		sp_language->modified = qfalse;
	}
}


int Language_GetIntegerValue(void)
{
	if (sp_language)
	{
		return sp_language->integer;
	}

	return 0;
}


// query function from font code
// 
qboolean Language_IsKorean(void)
{
	return (sp_language && sp_language->integer == SP_LANGUAGE_KOREAN) ? qtrue : qfalse;
}

qboolean Language_IsTaiwanese(void)
{
	return (sp_language && sp_language->integer == SP_LANGUAGE_TAIWANESE) ? qtrue : qfalse;
}

qboolean Language_IsJapanese(void)
{
	return (sp_language && sp_language->integer == SP_LANGUAGE_JAPANESE) ? qtrue : qfalse;
}


#endif

