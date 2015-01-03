/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// Filename:-	genericparser2.h


#ifndef GENERICPARSER2_H
#define GENERICPARSER2_H


// conditional expression is constant
// conversion from int to char, possible loss of data
// unreferenced inline funciton has been removed
#ifdef _MSC_VER
#pragma warning( disable : 4127 4244 4514 )


#ifdef DEBUG_LINKING
	#pragma message("...including GenericParser2.h")
#endif
#endif

#ifdef _JK2EXE
#define trap_Z_Malloc(x, y)		Z_Malloc(x,y,qtrue)
#define trap_Z_Free(x)			Z_Free(x)
#else
#define trap_Z_Malloc(x, y)		gi.Malloc(x,y,qtrue)
#define trap_Z_Free(x)			gi.Free(x)
#endif


class CTextPool;
class CGPObject;

class CTextPool
{
private:
	char		*mPool;
	CTextPool	*mNext;
	int			mSize, mUsed;

public:
	CTextPool(int initSize = 10240);
	~CTextPool(void);

	CTextPool	*GetNext(void) { return mNext; }
	void		SetNext(CTextPool *which) { mNext = which; }
	char		*GetPool(void) { return mPool; }
	int			GetUsed(void) { return mUsed; }

	char		*AllocText(const char *text, bool addNULL = true, CTextPool **poolPtr = 0);
};

void CleanTextPool(CTextPool *pool);

class CGPObject
{
protected:
	const char	*mName;
	CGPObject	*mNext, *mInOrderNext, *mInOrderPrevious;

public:
				CGPObject(const char *initName);

	const char	*GetName(void) { return mName; }

	CGPObject	*GetNext(void) { return mNext; }
	void		SetNext(CGPObject *which) { mNext = which; }
	CGPObject	*GetInOrderNext(void) { return mInOrderNext; }
	void		SetInOrderNext(CGPObject *which) { mInOrderNext = which; }
	CGPObject	*GetInOrderPrevious(void) { return mInOrderPrevious; }
	void		SetInOrderPrevious(CGPObject *which) { mInOrderPrevious = which; }

	bool		WriteText(CTextPool **textPool, const char *text);
};



class CGPValue : public CGPObject
{
private:
	CGPObject	*mList;

public:
					CGPValue(const char *initName, const char *initValue = 0);
					~CGPValue(void);

	CGPValue		*GetNext(void) { return (CGPValue *)mNext; }

	CGPValue		*Duplicate(CTextPool **textPool = 0);

	bool			IsList(void);
	const char		*GetTopValue(void);
	CGPObject		*GetList(void) { return mList; }
	void			AddValue(const char *newValue, CTextPool **textPool = 0);

	bool			Parse(char **dataPtr, CTextPool **textPool);

	bool			Write(CTextPool **textPool, int depth);
};



class CGPGroup : public CGPObject
{
private:
	CGPValue			*mPairs, *mInOrderPairs;
	CGPValue			*mCurrentPair;
	CGPGroup			*mSubGroups, *mInOrderSubGroups;
	CGPGroup			*mCurrentSubGroup;
	CGPGroup			*mParent;
	bool				mWriteable;

	void	SortObject(CGPObject *object, CGPObject **unsortedList, CGPObject **sortedList, 
					   CGPObject **lastObject);

public:
				CGPGroup(const char *initName = "Top Level", CGPGroup *initParent = 0);
				~CGPGroup(void);

	CGPGroup	*GetParent(void) { return mParent; }
	CGPGroup	*GetNext(void) { return (CGPGroup *)mNext; }
	int			GetNumSubGroups(void); 
	int			GetNumPairs(void);

	void		Clean(void); 
	CGPGroup	*Duplicate(CTextPool **textPool = 0, CGPGroup *initParent = 0);

	void		SetWriteable(const bool writeable) { mWriteable = writeable; }
	CGPValue	*GetPairs(void) { return mPairs; }
	CGPValue	*GetInOrderPairs(void) { return mInOrderPairs; }
	CGPGroup	*GetSubGroups(void) { return mSubGroups; }
	CGPGroup	*GetInOrderSubGroups(void) { return mInOrderSubGroups; }

	CGPValue	*AddPair(const char *name, const char *value, CTextPool **textPool = 0);
	void		AddPair(CGPValue *NewPair);
	CGPGroup	*AddGroup(const char *name, CTextPool **textPool = 0);
	void		AddGroup(CGPGroup *NewGroup);
	CGPGroup	*FindSubGroup(const char *name);
	bool		Parse(char **dataPtr, CTextPool **textPool);
	bool		Write(CTextPool **textPool, int depth);

	CGPValue	*FindPair(const char *key);
	const char	*FindPairValue(const char *key, const char *defaultVal = 0);
};

class CGenericParser2
{
private:
	CGPGroup		mTopLevel;
	CTextPool		*mTextPool;
	bool			mWriteable;

public:
	CGenericParser2(void);
	~CGenericParser2(void);

	void		SetWriteable(const bool writeable) { mWriteable = writeable; }
	CGPGroup	*GetBaseParseGroup(void) { return &mTopLevel; }

	bool	Parse(char **dataPtr, bool cleanFirst = true, bool writeable = false);
	bool	Parse(char *dataPtr, bool cleanFirst = true, bool writeable = false)
	{
		return Parse(&dataPtr, cleanFirst, writeable);
	}
	void	Clean(void);

	bool	Write(CTextPool *textPool);
};



// The following groups of routines are used for a C interface into GP2.
// C++ users should just use the objects as normally and not call these routines below
//

typedef		void	*TGenericParser2;
typedef		void	*TGPGroup;
typedef		void	*TGPValue;

// CGenericParser2 (void *) routines
TGenericParser2		GP_Parse(char **dataPtr, bool cleanFirst, bool writeable);
void				GP_Clean(TGenericParser2 GP2);
void				GP_Delete(TGenericParser2 *GP2);
TGPGroup			GP_GetBaseParseGroup(TGenericParser2 GP2);

// CGPGroup (void *) routines
const char	*GPG_GetName(TGPGroup GPG);
TGPGroup	GPG_GetNext(TGPGroup GPG);
TGPGroup	GPG_GetInOrderNext(TGPGroup GPG);
TGPGroup	GPG_GetInOrderPrevious(TGPGroup GPG);
TGPGroup	GPG_GetPairs(TGPGroup GPG);
TGPGroup	GPG_GetInOrderPairs(TGPGroup GPG);
TGPGroup	GPG_GetSubGroups(TGPGroup GPG);
TGPGroup	GPG_GetInOrderSubGroups(TGPGroup GPG);
TGPGroup	GPG_FindSubGroup(TGPGroup GPG, const char *name);
TGPValue	GPG_FindPair(TGPGroup GPG, const char *key);
const char	*GPG_FindPairValue(TGPGroup GPG, const char *key, const char *defaultVal);

// CGPValue (void *) routines
const char	*GPV_GetName(TGPValue GPV);
TGPValue	GPV_GetNext(TGPValue GPV);
TGPValue	GPV_GetInOrderNext(TGPValue GPV);
TGPValue	GPV_GetInOrderPrevious(TGPValue GPV);
bool		GPV_IsList(TGPValue GPV);
const char	*GPV_GetTopValue(TGPValue GPV);
TGPValue	GPV_GetList(TGPValue GPV);


#endif	// #ifndef GENERICPARSER2_H


//////////////////// eof /////////////////////

