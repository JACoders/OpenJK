#ifndef __STRIP_H
#define __STRIP_H


#pragma warning(disable:4510)	//default ctor could not be generated
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4610)	//user def ctor required
#pragma warning(disable:4663)

#pragma warning (push, 3)		//go back down to 3 for the stl include
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
#include <string>
#include <list>
#pragma warning (pop)
#pragma warning(disable:4503)	// decorated name length xceeded, name was truncated


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
	virtual bool	Load(char *&Data, int &Size );
};


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

	virtual bool	UnderstandToken(int token, char *data );
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

	virtual bool	UnderstandToken(char *&Data, int &Size, int token, char *data );
	virtual bool	Load(char *FileName );
	virtual bool	Load(char *Data, int &Size );
};


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


typedef struct sFlagPair
{
	int				Name;
	unsigned long	Value;
} tFlagPair;

extern sFlagPair FlagPairs[];
extern sFlagPair LanguagePairs[];


#endif // __STRIP_H
