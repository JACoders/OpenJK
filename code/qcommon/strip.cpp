/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#ifdef JK2_MODE
// this include must remain at the top of every CPP file
#include "../server/server.h"
#include "q_shared.h"
#include "qcommon.h"
#include "stringed_ingame.h"

#include <string>
#include <list>

cvar_t	*sp_language;
static cvar_t	*sp_show_strip;
static cvar_t	*sp_leet;

#define STRIP_VERSION	1

#define MAX_LANGUAGES	10
#define MAX_STRINGS		256
#define MAX_ID			255

/*enum
{
	SP_LANGUAGE_ENGLISH = 0,
	SP_LANGUAGE_FRENCH,
	SP_LANGUAGE_GERMAN,
	SP_LANGUAGE_BRITISH,
	SP_LANGUAGE_KOREAN,
	SP_LANGUAGE_TAIWANESE,
	SP_LANGUAGE_ITALIAN,
	SP_LANGUAGE_SPANISH,
	SP_LANGUAGE_JAPANESE,
	SP_LANGUAGE_10,
	SP_LANGUGAGE_MAX,
	SP_LANGUAGE_ALL = 255
};*/


#define SP_PACKAGE		0xff00
#define SP_STRING		0x00ff

#define SP_GET_PACKAGE(x) ( (x & SP_PACKAGE) >> 8 )

// Flags
#define SP_FLAG1				0x00000001	// CENTERED
#define SP_FLAG2				0x00000002
#define SP_FLAG3				0x00000004
#define SP_FLAG4				0x00000008
#define SP_FLAG5				0x00000010
#define	SP_FLAG6				0x00000020
#define	SP_FLAG7				0x00000040
#define	SP_FLAG8				0x00000080
#define	SP_FLAG9				0x00000100
#define SP_FLAG_ORIGINAL		0x00000200

// Registration
#define SP_REGISTER_CLIENT	 (0x01)
#define SP_REGISTER_SERVER	 (0x02)
#define SP_REGISTER_MENU	 (0x04)
#define SP_REGISTER_REQUIRED (0x08)


//======================================================================

class cStringPackageID
{
private:
	std::string	name;
	byte	reg;
public:
	cStringPackageID(const char *in_name, byte in_reg) { name = in_name; reg = in_reg; }
	const char *GetName(void) const { return(name.c_str()); }
	byte GetReg(void) const { return(reg); }
};


class cStringPackage
{
protected:
	unsigned char	ID;
	unsigned char	Registration;
	std::string			name;
	char			*Reference;

public:
					cStringPackage(const char *in, unsigned char initID = 0, char *initDescription = NULL, char *initReference = NULL);
	virtual			~cStringPackage(void);

	void			Register(unsigned char newRegistration) { Registration |= newRegistration; }
	bool			UnRegister(unsigned char oldRegistration) { Registration &= ~oldRegistration; return (Registration == 0); }
	bool			RegisteredOnServer(void) const { return(!!(Registration & SP_REGISTER_SERVER)); }
	byte			GetRegistration(void) const { return(Registration); }

	void			SetID(unsigned char newID) { ID = newID; }
	void			SetReference(char *newReference);

	unsigned char	GetID(void) { return ID; }
	char			*GetReference(void) { return Reference; }
	const char		*GetName(void) const { return(name.c_str()); }

	virtual bool	UnderstandToken(char *&Data, int &Size, int token, char *data );
#if 0
	virtual bool	Load(char *FileName );
#endif
	virtual bool	Load(char *Data, int &Size );
};


class cStringPackageSingle : public cStringPackage
{
private:
	cStringsSingle		Strings[MAX_STRINGS];
	std::map<std::string, int>	ReferenceTable;

public:
					cStringPackageSingle(const char *in, unsigned char initID = 0, char *initReference = NULL);
					~cStringPackageSingle(void);

	cStringsSingle	*FindString(int index) { return &Strings[index]; }
	cStringsSingle	*FindString(char *ReferenceLookup);
	int				FindStringID(const char *ReferenceLookup);

	virtual bool	UnderstandToken(char *&Data, int &Size, int token, char *data );
};


typedef struct sFlagPair
{
	int				Name;
	unsigned long	Value;
} tFlagPair;


enum
{
	TK_INVALID = -1,
	TK_TEXT_LANGUAGE1 = 0,
	TK_TEXT_LANGUAGE2,
	TK_TEXT_LANGUAGE3,
	TK_TEXT_LANGUAGE4,
	TK_TEXT_LANGUAGE5,
	TK_TEXT_LANGUAGE6,
	TK_TEXT_LANGUAGE7,
	TK_TEXT_LANGUAGE8,
	TK_TEXT_LANGUAGE9,
	TK_TEXT_LANGUAGE10,
	TK_VERSION,
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
	TK_END
};


const char *Tokens[TK_END] =
{
	"TEXT_LANGUAGE1",
	"TEXT_LANGUAGE2",
	"TEXT_LANGUAGE3",
	"TEXT_LANGUAGE4",
	"TEXT_LANGUAGE5",
	"TEXT_LANGUAGE6",
	"TEXT_LANGUAGE7",
	"TEXT_LANGUAGE8",
	"TEXT_LANGUAGE9",
	"TEXT_LANGUAGE10",
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
	"CONFIG"
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

bool cStrings::UnderstandToken(int token, char *data )
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


/************************************************************************************************
 * Load - Load the given string packet file
 *
 * inputs:
 *	buffer to load line in
 *	size of final text string area
 *	argument list of data needed by the string package
 *
 * return:
 *	done or not
 *
 ************************************************************************************************/
bool cStrings::Load(char *&Data, int &Size )
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
	while (UnderstandToken(token, data) )
	{
		GetLine(Data, Size, token, data);
	}

	if (token != TK_RIGHT_BRACE)
	{
		return false;
	}

	return true;
}



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

	// Following is for TESTING for SOF.
	if(sp_show_strip->value)
	{
		const char sDebugString[]="SP:";
		Dest = Text = new char[length + strlen(sDebugString)];
		strcpy(Dest,sDebugString);
		Dest += strlen(Dest);
	}
	else
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

	// bug fix for picky grammatical errors, replace "?." with "? "
	//
	while ((p=strstr(psText,"?."))!=NULL)
	{
		p[1] = ' ';
	}

	// StripEd and our print code don't support tabs...
	//
	while ((p=strchr(psText,0x09))!=NULL)
	{
		*p = ' ';
	}


	if (sp_leet->integer == 42)	// very specific test, so you won't hit it accidentally
	{
		char cReplace[]={	'o','0','l','1','e','3','a','4','s','5','t','7','i','!','h','#',
							'O','0','L','1','E','3','A','4','S','5','T','7','I','!','H','#'	// laziness because of strchr()
						};

		for (size_t i=0; i<sizeof(cReplace); i+=2)
		{
			while ((p=strchr(psText,cReplace[i]))!=NULL)
				*p = cReplace[i+1];
		}
	}
}

bool cStringsSingle::UnderstandToken(int token, char *data )
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
				else if (LanguagePair->Name == token && (signed) LanguagePair->Value == sp_language->integer)
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

			return cStrings::UnderstandToken(token, data );
//	}
}



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


bool cStringPackage::UnderstandToken(char *&Data, int &Size, int token, char *data )
{
	switch(token)
	{
		case TK_ID:
			ID = (unsigned char)atol(data);
			return true;

		case TK_CONFIG:
			return true;

		case TK_REFERENCE:
			SetReference(data);
			return true;
	}

	if (token == TK_INVALID)
	{
		return false;
	}

	return true;
}


#if 0
bool cStringPackage::Load(char *FileName )
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

	Load(buffer, Size );

	delete[] buffer;

	return true;
}
#endif

bool cStringPackage::Load(char *Data, int &Size )
{
	char	*token_data;
	int		token;

	GetLine(Data, Size, token, token_data);
	if (token != TK_VERSION || atol(token_data) != STRIP_VERSION)
	{
		return false;
	}

	GetLine(Data, Size, token, token_data);
	while (UnderstandToken(Data, Size, token, token_data) )
	{
		GetLine(Data, Size, token, token_data);
	}

	return true;
}



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
	std::map<std::string, int>::iterator	i;
	int							size;

	if (!Reference)
	{
		return -1;
	}

	size = strlen(Reference);
	if ((int)strlen(ReferenceLookup) < size+2)
	{
		return -1;
	}

	if (Q_stricmpn(ReferenceLookup, Reference, size))
	{
		return -1;
	}

	i = ReferenceTable.find(std::string(ReferenceLookup + size + 1));
	if (i != ReferenceTable.end())
	{
		return (*i).second;
	}

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
					ReferenceTable[std::string(ReferenceLookup)] = pos;
				}
			}
			return true;

		default:
			return cStringPackage::UnderstandToken(Data, Size, token, data);
	}
}


// A map of loaded string packages
std::map<std::string, cStringPackageSingle *>		JK2SP_ListByName;
std::map<byte, cStringPackageSingle *>		JK2SP_ListByID;


// Registration
/************************************************************************************************
 * JK2SP_Register : Load given string package file. Register it.
 *
 * Inputs:
 *	Package File name
 *	Registration flag
 *
 * Return:
 *	success/fail
 *
 ************************************************************************************************/
qboolean JK2SP_Register(const char *inPackage, unsigned char Registration)
{
	char											*buffer;
	char											Package[MAX_QPATH];
	int												size;
	cStringPackageSingle							*new_sp;
	std::map<std::string, cStringPackageSingle *>::iterator	i;


	assert(JK2SP_ListByName.size() == JK2SP_ListByID.size());

	Q_strncpyz(Package, inPackage, MAX_QPATH);
	Q_strupr(Package);

	i = JK2SP_ListByName.find(Package);
	if (i != JK2SP_ListByName.end())
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
			return qfalse;
		}

		// Create the new string package
		new_sp = new cStringPackageSingle(Package);
		new_sp->Load(buffer, size );
		FS_FreeFile(buffer);

		if (Registration & SP_REGISTER_CLIENT)
		{
			Com_DPrintf(S_COLOR_YELLOW "JK2SP_Register: Registered client string package '%s' with ID %02x\n", Package, (int)new_sp->GetID());
		}
		else
		{
			Com_DPrintf(S_COLOR_YELLOW "JK2SP_Register: Registered string package '%s' with ID %02x\n", Package, (int)new_sp->GetID());
		}

		// Insert into the name vs package map
		JK2SP_ListByName[Package] = new_sp;
		// Insert into the id vs package map
		JK2SP_ListByID[new_sp->GetID()] = new_sp;
	}
	// Or in the new registration data
	new_sp->Register(Registration);

//	return new_sp;
	return qtrue;
}


// Unload all packages with the relevant registration bits
void JK2SP_Unload(unsigned char Registration)
{
	std::map<std::string, cStringPackageSingle *>::iterator	i, next;
	std::map<byte, cStringPackageSingle *>::iterator		id;

	assert(JK2SP_ListByName.size() == JK2SP_ListByID.size());

	for(i = JK2SP_ListByName.begin(); i != JK2SP_ListByName.end(); i = next)
	{
		next = i;
		++next;

		if ((*i).second->UnRegister(Registration))
		{
			Com_DPrintf(S_COLOR_YELLOW "JK2SP_UnRegister: Package '%s' with ID %02x\n", (*i).first.c_str(), (int)(*i).second->GetID());

			id = JK2SP_ListByID.find((*i).second->GetID());
			JK2SP_ListByID.erase(id);
			delete i->second;
			JK2SP_ListByName.erase(i);
		}
	}

}

// Direct string functions

int JK2SP_GetStringID(const char *inReference)
{
	std::map<unsigned char,cStringPackageSingle *>::iterator	i;
	int													ID;
	char Reference[MAX_QPATH];
	Q_strncpyz(Reference, inReference, MAX_QPATH);
	Q_strupr(Reference);

	for(i = JK2SP_ListByID.begin(); i != JK2SP_ListByID.end(); ++i)
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
 * JK2SP_GetString
 *
 * inputs:
 *	ID of the string package
 *
 * return:
 *	pointer to desired String Package
 *
 ************************************************************************************************/
cStringsSingle *JK2SP_GetString(unsigned short ID)
{
	cStringPackageSingle								*sp;
	cStringsSingle										*string;
	std::map<unsigned char,cStringPackageSingle *>::iterator	i;

	i = JK2SP_ListByID.find(SP_GET_PACKAGE(ID));
	if (i == JK2SP_ListByID.end())
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

cStringsSingle *JK2SP_GetString(const char *Reference)
{
	int	index;

	index = JK2SP_GetStringID(Reference);
	if (index == -1)
	{
		return NULL;
	}

	return JK2SP_GetString(index);
}

#ifdef _DEBUG
// needed to add this to query which SP references the menus used	-Ste.
//
const char *JK2SP_GetReferenceText(unsigned short ID, const char *&psPackageName, const char *&psPackageReference, const char *&psText)
{
	cStringPackageSingle *sp;
	std::map<unsigned char,cStringPackageSingle *>::iterator	i;

	i = JK2SP_ListByID.find(SP_GET_PACKAGE(ID));
	if (i == JK2SP_ListByID.end())
	{
		assert(0);
		return NULL;
	}

	cStringsSingle *string;

	sp = (*i).second;
	string = sp->FindString(ID & SP_STRING);

	if (!string)
	{
		assert(0);
		return NULL;
	}

	psPackageName = sp->GetName();
	psPackageReference	= sp->GetReference();
	psText = string->GetText();
	if (!psText)
		 psText = "";
	return string->GetReference();
}
#endif

const char *JK2SP_GetStringText(unsigned short ID)
{
	cStringsSingle			*string;
	const char					*value;

	string = JK2SP_GetString(ID);

	value = string->GetText();
	if (!value)
	{
		value = "";
	}

	return value;
}

const char *JK2SP_GetStringTextString(const char *Reference)
{
	int	index;

	index = JK2SP_GetStringID(Reference);
	if (index == -1)
	{
		return "";
	}

	return JK2SP_GetStringText(index);
}


static void JK2SP_UpdateLanguage(void)
{
	std::map<unsigned char, cStringPackageSingle *>::iterator	it;
	std::list<cStringPackageID>									sps;
	std::list<cStringPackageID>::iterator						spit;

	// Grab all SP ids
	for(it = JK2SP_ListByID.begin(); it != JK2SP_ListByID.end(); ++it)
	{
		sps.push_back(cStringPackageID((*it).second->GetName(), (*it).second->GetRegistration()));
	}
	// Clear out all pointers
	JK2SP_Unload(SP_REGISTER_CLIENT | SP_REGISTER_SERVER | SP_REGISTER_MENU | SP_REGISTER_REQUIRED);

	// Reinitialise with new language
	for(spit = sps.begin(); spit != sps.end(); ++spit)
	{
		JK2SP_Register((*spit).GetName(), (*spit).GetReg());
	}
	sps.clear();
}

void JK2SP_Init(void)
{
	sp_language = Cvar_Get("sp_language", va("%d", SP_LANGUAGE_ENGLISH), CVAR_ARCHIVE | CVAR_NORESTART);
	sp_show_strip = Cvar_Get ("sp_show_strip", "0", 0);		// don't switch this on!!!!!!, test only (apparently)
	sp_leet = Cvar_Get ("sp_leet", "0", CVAR_ROM);		// do NOT leave this on in final product!!!!  (only works when == 42 anyway ;-)

//	Cvar_Set("sp_language", va("%d", SP_LANGUAGE_JAPANESE));	// stetest, do NOT leave in

	JK2SP_UpdateLanguage();
	sp_language->modified = qfalse;

	JK2SP_Register("con_text", SP_REGISTER_REQUIRED);	//reference is CON_TEXT
}

// called in Com_Frame, so don't take up any time! (can also be called during dedicated)
//
void JK2SP_CheckForLanguageUpdates(void)
{
	if (sp_language && sp_language->modified)
	{
		JK2SP_UpdateLanguage();	// force language package to reload
		sp_language->modified = qfalse;
	}
}

#endif
