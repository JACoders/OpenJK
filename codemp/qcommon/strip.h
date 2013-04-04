#ifndef __STRIP_H
#define __STRIP_H
/*
#ifndef _SOF_

#pragma warning(disable:4786) // Or STL will generate ugly warnings.

#include <string>

using namespace std;
#endif
*/

#pragma warning (push, 3)	//go back down to 3 for the stl include
#include <string>
#include <map>
#include <list>
#pragma warning (pop)

using namespace std;


#define STRIP_VERSION	1

#define MAX_LANGUAGES	10
#define MAX_STRINGS		256
#define MAX_ID			255

enum
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
};


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


class cCriteria
{
public:
	int					WhichLanguage;

	cCriteria(int initWhichLanguage = SP_LANGUAGE_ALL);
};

#ifdef _STRIPED_

class cStringPackageED;

class cCriteriaED : public cCriteria
{
public:
	cStringPackageED	*Merge;

	cCriteriaED(int initWhichLanguage = SP_LANGUAGE_ALL, cStringPackageED *initMerge = NULL);
};


#endif



class cStrings
{
private:
	unsigned int	Flags;
	char			*Reference;
	
public:
					cStrings(unsigned int initFlags = 0, char *initReference = NULL);
	virtual			~cStrings(void);

	virtual void	Clear(void);

	void			SetFlags(unsigned int newFlags);
	void			SetReference(char *newReference);

	unsigned int	GetFlags(void) { return Flags; }
	char			*GetReference(void) { return Reference; }

	virtual bool	UnderstandToken(int token, char *data );
	virtual bool	Load(char *&Data, int &Size);

	virtual bool	SubSave(FILE *FH);
	bool			Save(FILE *FH );
};

#ifdef _STRIPED_

class cStringsED : public cStrings
{
private:
	char			*Reference;
	char			*Text[MAX_LANGUAGES];
	char			*Notes;
	bool			Used;
	
public:
					cStringsED(unsigned int initFlags = 0, char *initReference = NULL, char *initNotes = NULL);
	virtual			~cStringsED();

	virtual void	Clear(void);

	void			SetUsed(bool newUsed = true) { Used = newUsed; }
	void			SetText(int index, char *newText);
	void			SetNotes(char *newNotes);

	bool			GetUsed(void) { return Used; }
	char			*GetText(int index) { return Text[index]; }
	char			*GetNotes(void) { return Notes; }

	virtual bool	UnderstandToken(int token, char *data, cCriteria &Criteria);
	virtual bool	SubSave(FILE *FH, cCriteria &Criteria);
	virtual bool	Load(char *&Data, int &Size, cCriteria &Criteria);
};

#endif

#ifndef _STRIPED_

class cStringsSingle : public cStrings
{
private:
	char			*Text;

	virtual void	Clear(void);
	void			SetText(const char *newText);

public:
					cStringsSingle(unsigned int initFlags = 0, char *initReference = NULL);
	virtual			~cStringsSingle();

	char			*GetText(void) { return Text; }
	virtual bool	UnderstandToken(int token, char *data);
};





//======================================================================

class cStringPackageID
{
private:
	string	name;
	byte	reg;
public:
	cStringPackageID(const char *in_name, byte in_reg) { name = in_name; reg = in_reg; }
	const char *GetName(void) const { return(name.c_str()); }
	byte GetReg(void) const { return(reg); }
};
#endif

class cStringPackage
{
protected:
	unsigned char	ID;
	unsigned char	Registration;
	string			name;
	char			*Reference;

public:
					cStringPackage(const char *in, unsigned char initID = 0, char *initDescription = NULL, char *initReference = NULL);
					~cStringPackage(void);

	void			Register(unsigned char newRegistration) { Registration |= newRegistration; }
	bool			UnRegister(unsigned char oldRegistration) { Registration &= ~oldRegistration; return (Registration == 0); }
	bool			RegisteredOnServer(void) const { return(!!(Registration & SP_REGISTER_SERVER)); }
	byte			GetRegistration(void) const { return(Registration); }

	void			SetID(unsigned char newID) { ID = newID; }
	void			SetReference(char *newReference);

	unsigned char	GetID(void) { return ID; }
	char			*GetReference(void) { return Reference; }
	const char		*GetName(void) const { return(name.c_str()); }

	virtual bool	UnderstandToken(char *&Data, int &Size, int token, char *data);
	virtual bool	SubSave(FILE *FH);
	bool			Save(char *FileName);
	virtual bool	Load(char *FileName);
	virtual bool	Load(char *Data, int &Size);
};

#ifdef _STRIPED_

class cStringPackageED : public cStringPackage
{
private:
	cStringsED		Strings[MAX_STRINGS];
	char			*Description;

public:
					cStringPackageED(unsigned char initID = 0, char *initDescription = NULL, char *initReference = NULL);
					~cStringPackageED(void);

	void			SetDescription(char *newDescription);

	char			*GetDescription(void) { return Description; }

	cStringsED		*FindString(int &index);
	void			ClearString(int index);

	virtual bool	UnderstandToken(char *&Data, int &Size, int token, char *data, cCriteria &Criteria);
	virtual bool	SubSave(FILE *FH, cCriteria &Criteria);
	bool			GenerateCHeader(char *FileName);
	bool			GenerateDSHeader(char *FileName);
};

#endif

#ifndef _STRIPED_

class cStringPackageSingle : public cStringPackage
{
private:
	cStringsSingle		Strings[MAX_STRINGS];
	map<string, int>	ReferenceTable;

public:
					cStringPackageSingle(const char *in, unsigned char initID = 0, char *initReference = NULL);
					~cStringPackageSingle(void);

	cStringsSingle	*FindString(int index) { return &Strings[index]; }
	cStringsSingle	*FindString(char *ReferenceLookup);
	int				FindStringID(const char *ReferenceLookup);

	virtual bool	UnderstandToken(char *&Data, int &Size, int token, char *data );
};

#endif



typedef struct sFlagPair
{
	int				Name;
	unsigned long	Value;
} tFlagPair;

extern sFlagPair FlagPairs[];
extern sFlagPair LanguagePairs[];

#ifdef _STRIPED_

#define LANGUAGELIST_MAX	16
#define LANGUAGE_LENGTH		64

#define FLAGLIST_MAX	16
#define FLAG_LENGTH		32

extern cStringPackageED *StringPackage;
extern char		LanguageList[LANGUAGELIST_MAX][LANGUAGE_LENGTH];
extern char		FlagList[FLAGLIST_MAX][FLAG_LENGTH];

#endif

#ifndef _STRIPED_

// Registration
cStringPackageSingle	*SP_Register(const char *Package, unsigned char Registration);
qboolean				SP_RegisterServer(const char *Package);
void					SP_Unload(unsigned char Registration);

// Direct string functions
int						SP_GetStringID(const char *Reference);
cStringsSingle			*SP_GetString(unsigned short ID);
cStringsSingle			*SP_GetString(const char *Reference);
const char				*SP_GetStringText(unsigned short ID);
const char				*SP_GetStringTextString(const char *Reference);

// Initialization
void					SP_Init(void);

#endif

extern int Language_GetIntegerValue(void);

#endif // __STRIP_H
