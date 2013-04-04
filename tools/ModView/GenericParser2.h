#pragma once
#if !defined(GENERICPARSER2_H_INC)
#define GENERICPARSER2_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including GenericParser2.h")
#endif

#include "disablewarnings.h"

#ifdef USE_LOCAL_GENERICPARSER
#include <memory.h>
#include <malloc.h>
#include <string.h>

#define trap_Z_Malloc(x, y)		malloc(x)
#define trap_Z_Free(x)			free(x)

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

	char		*AllocText(char *text, bool addNULL = true, CTextPool **poolPtr = 0);
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
// CGenericParser2 (void *) routines
void		*GP_Parse(char **dataPtr, bool cleanFirst, bool writeable);
void		GP_Clean(void *GP2);
void		GP_Delete(void **GP2);
void		*GP_GetBaseParseGroup(void *GP2);

// CGPGroup (void *) routines
const char	*GPG_GetName(void *GPG);
void		*GPG_GetNext(void *GPG);
void		*GPG_GetInOrderNext(void *GPG);
void		*GPG_GetInOrderPrevious(void *GPG);
void		*GPG_GetPairs(void *GPG);
void		*GPG_GetInOrderPairs(void *GPG);
void		*GPG_GetSubGroups(void *GPG);
void		*GPG_GetInOrderSubGroups(void *GPG);
void		*GPG_FindSubGroup(void *GPG, const char *name);
const char	*GPG_FindPairValue(void *GPG, const char *key, const char *defaultVal);

// CGPValue (void *) routines
const char	*GPV_GetName(void *GPV);
void		*GPV_GetNext(void *GPV);
void		*GPV_GetInOrderNext(void *GPV);
void		*GPV_GetInOrderPrevious(void *GPV);
bool		GPV_IsList(void *GPV);
const char	*GPV_GetTopValue(void *GPV);
void		*GPV_GetList(void *GPV);



#endif // GENERICPARSER2_H_INC
