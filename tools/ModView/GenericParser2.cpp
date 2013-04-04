// this include must remain at the top of every CPP file

#include "common_headers.h"
#include "GenericParser2.h"


#define MAX_TOKEN_SIZE	1024
static char	token[MAX_TOKEN_SIZE];

static char *GetToken(char **text, bool allowLineBreaks, bool readUntilEOL = false)
{
	char	*pointer = *text;
	int		length = 0;
	int		c = 0;
	bool	foundLineBreak;

	token[0] = 0;
	if (!pointer)
	{
		return token;
	}

	while(1)
	{
		foundLineBreak = false;
		while(1)
		{
			c = *pointer;
			if (c > ' ')
			{
				break;
			}
			if (!c)
			{
				*text = 0;
				return token;
			}
			if (c == '\n')
			{
				foundLineBreak = true;
			}
			pointer++;
		}
		if (foundLineBreak && !allowLineBreaks)
		{
			*text = pointer;
			return token;
		}

		c = *pointer;

		// skip single line comment
		if (c == '/' && pointer[1] == '/')
		{
			pointer += 2;
			while (*pointer && *pointer != '\n') 
			{
				pointer++;
			}
		}
		// skip multi line comments
		else if (c == '/' && pointer[1] == '*') 
		{
			pointer += 2;
			while (*pointer && (*pointer != '*' || pointer[1] != '/')) 
			{
				pointer++;
			}
			if (*pointer) 
			{
				pointer += 2;
			}
		}
		else
		{	// found the start of a token
			break;
		}
	}

	if (c == '\"')
	{	// handle a string
		pointer++;
		while (1)
		{
			c = *pointer++;
			if (c == '\"')
			{
//				token[length++] = c;
				break;
			}
			else if (!c)
			{
				break;
			}
			else if (length < MAX_TOKEN_SIZE)
			{
				token[length++] = c;
			}
		}
	}
	else if (readUntilEOL)
	{
		// absorb all characters until EOL
		while(c != '\n' && c != '\r')
		{
			if (c == '/' && ((*(pointer+1)) == '/' || (*(pointer+1)) == '*'))
			{
				break;
			}

			if (length < MAX_TOKEN_SIZE)
			{
				token[length++] = c;
			}
			pointer++;
			c = *pointer;
		}
		// remove trailing white space
		while(length && token[length-1] < ' ')
		{
			length--;
		}
	}
	else
	{
		while(c > ' ')
		{
			if (length < MAX_TOKEN_SIZE)
			{
				token[length++] = c;
			}
			pointer++;
			c = *pointer;
		}
	}

	if (token[0] == '\"')
	{	// remove start quote
		length--;
		memmove(token, token+1, length);

		if (length && token[length-1] == '\"')
		{	// remove end quote
			length--;
		}
	}

	if (length >= MAX_TOKEN_SIZE)
	{
		length = 0;
	}
	token[length] = 0;
	*text = (char *)pointer;

	return token;
}




CTextPool::CTextPool(int initSize) :
	mNext(0),
	mSize(initSize),
	mUsed(0)
{
#ifdef _EXE
	mPool = (char *)Z_Malloc(mSize, TAG_GP2);
//	mPool = (char *)Z_Malloc(mSize, TAG_TEXTPOOL, qtrue);
#else
	mPool = (char *)trap_Z_Malloc(mSize, TAG_GP2);
#endif
}

CTextPool::~CTextPool(void)
{
#ifdef _EXE
	Z_Free(mPool);
#else
	trap_Z_Free(mPool);
#endif
}

char *CTextPool::AllocText(char *text, bool addNULL, CTextPool **poolPtr)
{
	int	length = strlen(text) + (addNULL ? 1 : 0);

	if (mUsed + length + 1> mSize)
	{	// extra 1 to put a null on the end
		if (poolPtr)
		{
			(*poolPtr)->SetNext(new CTextPool(mSize));
			*poolPtr = (*poolPtr)->GetNext();

			return (*poolPtr)->AllocText(text, addNULL);
		}

		return 0;
	}

	strcpy(mPool + mUsed, text);
	mUsed += length;
	mPool[mUsed] = 0;

	return mPool + mUsed - length;
}

void CleanTextPool(CTextPool *pool)
{
	CTextPool *next;

	while(pool)
	{
		next = pool->GetNext();
		delete pool;
		pool = next;
	}
}







CGPObject::CGPObject(const char *initName) :
	mName(initName),
	mNext(0),
	mInOrderNext(0),
	mInOrderPrevious(0)
{
}

bool CGPObject::WriteText(CTextPool **textPool, const char *text)
{
   if (strchr(text, ' ') || !text[0])
   {
	   (*textPool)->AllocText("\"", false, textPool);
	   (*textPool)->AllocText((char *)text, false, textPool);
	   (*textPool)->AllocText("\"", false, textPool);
   }
   else
   {
	   (*textPool)->AllocText((char *)text, false, textPool);
   }

   return true;
}


	
	
	






	
	

CGPValue::CGPValue(const char *initName, const char *initValue) :
	CGPObject(initName),
	mList(0)
{
	if (initValue)
	{
		AddValue(initValue);
	}
}

CGPValue::~CGPValue(void)
{
	CGPObject	*next;

	while(mList)
	{
		next = mList->GetNext();
		delete mList;
		mList = next;
	}
}

CGPValue *CGPValue::Duplicate(CTextPool **textPool)
{
	CGPValue	*newValue;
	CGPObject	*iterator;
	char		*name;

	if (textPool)
	{
		name = (*textPool)->AllocText((char *)mName, true, textPool);
	}
	else
	{
		name = (char *)mName;
	}

	newValue = new CGPValue(name);
	iterator = mList;
	while(iterator)
	{
		if (textPool)
		{
			name = (*textPool)->AllocText((char *)iterator->GetName(), true, textPool);
		}
		else
		{
			name = (char *)iterator->GetName();
		}
		newValue->AddValue(name);
		iterator = iterator->GetNext();
	}

	return newValue;
}

bool CGPValue::IsList(void)
{
	if (!mList || !mList->GetNext())
	{
		return false;
	}

	return true;
}

const char *CGPValue::GetTopValue(void) 
{ 
	if (mList)
	{
		return mList->GetName();
	}

	return 0;
}

void CGPValue::AddValue(const char *newValue, CTextPool **textPool)
{
	if (textPool)
	{
		newValue = (*textPool)->AllocText((char *)newValue, true, textPool);
	}

	if (mList == 0)
	{
		mList = new CGPObject(newValue);
		mList->SetInOrderNext(mList);
	}
	else
	{
		mList->GetInOrderNext()->SetNext(new CGPObject(newValue));
		mList->SetInOrderNext(mList->GetInOrderNext()->GetNext());
	}
}

bool CGPValue::Parse(char **dataPtr, CTextPool **textPool)
{
	char		*token;
	char		*value;

	while(1)
	{
		token = GetToken(dataPtr, true, true);

		if (!token[0])
		{	// end of data - error!
			return false;
		}
		else if (strcmpi(token, "]") == 0)
		{	// ending brace for this list
			break;
		}

		value = (*textPool)->AllocText(token, true, textPool);
		AddValue(value);
	}

	return true;
}

bool CGPValue::Write(CTextPool **textPool, int depth)
{
	int				i;
	CGPObject	*next;

	if (!mList)
	{
		return true;
	}

	for(i=0;i<depth;i++)
	{
		(*textPool)->AllocText("\t", false, textPool);
	}

	WriteText(textPool, mName);

	if (!mList->GetNext())
	{
		(*textPool)->AllocText("\t\t", false, textPool);
		mList->WriteText(textPool, mList->GetName());
		(*textPool)->AllocText("\r\n", false, textPool);
	}
	else
	{
		(*textPool)->AllocText("\r\n", false, textPool);

		for(i=0;i<depth;i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("[\r\n", false, textPool);

		next = mList;
		while(next)
		{
			for(i=0;i<depth+1;i++)
			{
				(*textPool)->AllocText("\t", false, textPool);
			}
			mList->WriteText(textPool, next->GetName());
			(*textPool)->AllocText("\r\n", false, textPool);

			next = next->GetNext();
		}

		for(i=0;i<depth;i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("]\r\n", false, textPool);
	}

	return true;
}
















CGPGroup::CGPGroup(const char *initName, CGPGroup *initParent) :
	CGPObject(initName),
	mPairs(0),
	mInOrderPairs(0),
	mCurrentPair(0),
	mSubGroups(0),
	mInOrderSubGroups(0),
	mCurrentSubGroup(0),
	mParent(initParent),
	mWriteable(false)
{
}

CGPGroup::~CGPGroup(void)
{
	Clean();
}

int CGPGroup::GetNumSubGroups(void)
{
	int			count;
	CGPGroup	*group;

	count = 0;
	group = mSubGroups;
	do
	{
		count++;
		group = (CGPGroup *)group->GetNext();
	}
	while(group);

	return(count);
}

int CGPGroup::GetNumPairs(void)
{
	int			count;
	CGPValue	*pair;

	count = 0;
	pair = mPairs;
	do
	{
		count++;
		pair = (CGPValue *)pair->GetNext();
	}
	while(pair);

	return(count);
}

void CGPGroup::Clean(void)
{
	while(mPairs)
	{
		mCurrentPair = (CGPValue *)mPairs->GetNext();
		delete mPairs;
		mPairs = mCurrentPair;
	}

	while(mSubGroups)
	{
		mCurrentSubGroup = (CGPGroup *)mSubGroups->GetNext();
		delete mSubGroups;
		mSubGroups = mCurrentSubGroup;
	}

	mPairs = mInOrderPairs = mCurrentPair = 0;
	mSubGroups = mInOrderSubGroups = mCurrentSubGroup = 0;
	mParent = 0;
	mWriteable = false;
}

CGPGroup *CGPGroup::Duplicate(CTextPool **textPool, CGPGroup *initParent)
{
	CGPGroup	*newGroup, *subSub, *newSub;
	CGPValue	*newPair, *subPair;
	char		*name;

	if (textPool)
	{
		name = (*textPool)->AllocText((char *)mName, true, textPool);
	}
	else
	{
		name = (char *)mName;
	}

	newGroup = new CGPGroup(name);

	subSub = mSubGroups;
	while(subSub)
	{
		newSub = subSub->Duplicate(textPool, newGroup);
		newGroup->AddGroup(newSub);

		subSub = (CGPGroup *)subSub->GetNext();
	}

	subPair = mPairs;
	while(subPair)
	{
		newPair = subPair->Duplicate(textPool);
		newGroup->AddPair(newPair);

		subPair = (CGPValue *)subPair->GetNext();
	}

	return newGroup;
}

void CGPGroup::SortObject(CGPObject *object, CGPObject **unsortedList, CGPObject **sortedList,
							 CGPObject **lastObject)
{
	CGPObject	*test, *last;

	if (!*unsortedList)
	{
		*unsortedList = *sortedList = object;
	}
	else
	{
		(*lastObject)->SetNext(object);

		test = *sortedList;
		last = 0;
		while(test)
		{
			if (strcmpi(object->GetName(), test->GetName()) < 0)
			{
				break;
			}

			last = test;
			test = test->GetInOrderNext();
		}

		if (test)
		{
			test->SetInOrderPrevious(object);
			object->SetInOrderNext(test);
		}
		if (last)
		{
			last->SetInOrderNext(object);
			object->SetInOrderPrevious(last);
		}
		else
		{
			*sortedList = object;
		}
	}

	*lastObject = object;
}

CGPValue *CGPGroup::AddPair(const char *name, const char *value, CTextPool **textPool)
{
	CGPValue	*newPair;

	if (textPool)
	{
		name = (*textPool)->AllocText((char *)name, true, textPool);
		if (value)
		{
			value = (*textPool)->AllocText((char *)value, true, textPool);
		}
	}

	newPair = new CGPValue(name, value);

	AddPair(newPair);

	return newPair;
}

void CGPGroup::AddPair(CGPValue *NewPair)
{
	SortObject(NewPair, (CGPObject **)&mPairs, (CGPObject **)&mInOrderPairs, 
		(CGPObject **)&mCurrentPair);
}

CGPGroup *CGPGroup::AddGroup(const char *name, CTextPool **textPool)
{
	CGPGroup	*newGroup;

	if (textPool)
	{
		name = (*textPool)->AllocText((char *)name, true, textPool);
	}

	newGroup = new CGPGroup(name);

	AddGroup(newGroup);

	return newGroup;
}
	
void CGPGroup::AddGroup(CGPGroup *NewGroup)
{
	SortObject(NewGroup, (CGPObject **)&mSubGroups, (CGPObject **)&mInOrderSubGroups, 
		(CGPObject **)&mCurrentSubGroup);
}
	
CGPGroup *CGPGroup::FindSubGroup(const char *name)
{
	CGPGroup	*group;

	group = mSubGroups;
	while(group)
	{
		if(!stricmp(name, group->GetName()))
		{
			return(group);
		}
		group = (CGPGroup *)group->GetNext();
	}
	return(NULL);
}

bool CGPGroup::Parse(char **dataPtr, CTextPool **textPool)
{
	char		*token;
	char		lastToken[MAX_TOKEN_SIZE];
	CGPGroup	*newSubGroup;
	CGPValue	*newPair;

	while(1)
	{
		token = GetToken(dataPtr, true);

		if (!token[0])
		{	// end of data - error!
			if (mParent)
			{
				return false;
			}
			else
			{
				break;
			}
		}
		else if (strcmpi(token, "}") == 0)
		{	// ending brace for this group
			break;
		}

		strcpy(lastToken, token);

		// read ahead to see what we are doing
		token = GetToken(dataPtr, true, true);
		if (strcmpi(token, "{") == 0)
		{	// new sub group
			newSubGroup = AddGroup(lastToken, textPool);
			newSubGroup->SetWriteable(mWriteable);
			if (!newSubGroup->Parse(dataPtr, textPool))
			{
				return false;
			}
		}
		else if (strcmpi(token, "[") == 0)
		{	// new pair list
			newPair = AddPair(lastToken, 0, textPool);
			if (!newPair->Parse(dataPtr, textPool))
			{
				return false;
			}
		}
		else
		{	// new pair
			AddPair(lastToken, token, textPool);
		}
	}

	return true;
}

bool CGPGroup::Write(CTextPool **textPool, int depth)
{
	int				i;
	CGPValue		*mPair = mPairs;
	CGPGroup		*mSubGroup = mSubGroups;

	if (depth >= 0)
	{
		for(i=0;i<depth;i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		WriteText(textPool, mName);
		(*textPool)->AllocText("\r\n", false, textPool);
		
		for(i=0;i<depth;i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("{\r\n", false, textPool);
	}

	while(mPair)
	{
		mPair->Write(textPool, depth+1);
		mPair = (CGPValue *)mPair->GetNext();
	}

	while(mSubGroup)
	{
		mSubGroup->Write(textPool, depth+1);
		mSubGroup = (CGPGroup *)mSubGroup->GetNext();
	}

	if (depth >= 0)
	{
		for(i=0;i<depth;i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("}\r\n", false, textPool);
	}

	return true;
}

const char *CGPGroup::FindPairValue(const char *key, const char *defaultVal)
{
	CGPValue		*mPair = mPairs;

	while(mPair)
	{
		if (strcmpi(mPair->GetName(), key) == 0)
		{
			return mPair->GetTopValue();
		}

		mPair = (CGPValue *)mPair->GetNext();
	}

	return defaultVal;
}















CGenericParser2::CGenericParser2(void) :
	mTextPool(0),
	mWriteable(false)
{
}

CGenericParser2::~CGenericParser2(void)
{
	Clean();
}

bool CGenericParser2::Parse(char **dataPtr, bool cleanFirst, bool writeable)
{
	CTextPool	*topPool;

	if (cleanFirst)
	{
		Clean();
	}

	if (!mTextPool)
	{
		mTextPool = new CTextPool;
	}

	SetWriteable(writeable);
	mTopLevel.SetWriteable(writeable);
	topPool = mTextPool;
	return mTopLevel.Parse(dataPtr, &topPool);
}

void CGenericParser2::Clean(void)
{
	mTopLevel.Clean();

	CleanTextPool(mTextPool);
	mTextPool = 0;
}

bool CGenericParser2::Write(CTextPool *textPool)
{
	return mTopLevel.Write(&textPool, -1);
}









// The following groups of routines are used for a C interface into GP2.
// C++ users should just use the objects as normally and not call these routines below
//
// CGenericParser2 (void *) routines
void *GP_Parse(char **dataPtr, bool cleanFirst, bool writeable)
{
	CGenericParser2		*parse;

	parse = new CGenericParser2;
	if (parse->Parse(dataPtr, cleanFirst, writeable))
	{
		return parse;
	}

	delete parse;
	return 0;
}

void GP_Clean(void *GP2)
{
	if (!GP2)
	{
		return;
	}

	((CGenericParser2 *)GP2)->Clean();
}

void GP_Delete(void **GP2)
{
	if (!GP2 || !(*GP2))
	{
		return;
	}

	delete ((CGenericParser2 *)(*GP2));
	(*GP2) = 0;
}

void *GP_GetBaseParseGroup(void *GP2)
{
	if (!GP2)
	{
		return 0;
	}
	
	return ((CGenericParser2 *)GP2)->GetBaseParseGroup();
}




// CGPGroup (void *) routines
const char	*GPG_GetName(void *GPG)
{
	if (!GPG)
	{
		return "";
	}

	return ((CGPGroup *)GPG)->GetName();
}

void *GPG_GetNext(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetNext();
}

void *GPG_GetInOrderNext(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetInOrderNext();
}

void *GPG_GetInOrderPrevious(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetInOrderPrevious();
}

void *GPG_GetPairs(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetPairs();
}

void *GPG_GetInOrderPairs(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetInOrderPairs();
}

void *GPG_GetSubGroups(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetSubGroups();
}

void *GPG_GetInOrderSubGroups(void *GPG)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->GetInOrderSubGroups();
}

void *GPG_FindSubGroup(void *GPG, const char *name)
{
	if (!GPG)
	{
		return 0;
	}

	return ((CGPGroup *)GPG)->FindSubGroup(name);
}

const char	*GPG_FindPairValue(void *GPG, const char *key, const char *defaultVal)
{
	if (!GPG)
	{
		return defaultVal;
	}

	return ((CGPGroup *)GPG)->FindPairValue(key, defaultVal);
}




// CGPValue (void *) routines
const char	*GPV_GetName(void *GPV)
{
	if (!GPV)
	{
		return "";
	}

	return ((CGPValue *)GPV)->GetName();
}

void *GPV_GetNext(void *GPV)
{
	if (!GPV)
	{
		return 0;
	}

	return ((CGPValue *)GPV)->GetNext();
}

void *GPV_GetInOrderNext(void *GPV)
{
	if (!GPV)
	{
		return 0;
	}

	return ((CGPValue *)GPV)->GetInOrderNext();
}

void *GPV_GetInOrderPrevious(void *GPV)
{
	if (!GPV)
	{
		return 0;
	}

	return ((CGPValue *)GPV)->GetInOrderPrevious();
}

bool GPV_IsList(void *GPV)
{
	if (!GPV)
	{
		return 0;
	}

	return ((CGPValue *)GPV)->IsList();
}

const char *GPV_GetTopValue(void *GPV)
{
	if (!GPV)
	{
		return "";
	}

	return ((CGPValue *)GPV)->GetTopValue();
}

void *GPV_GetList(void *GPV)
{
	if (!GPV)
	{
		return 0;
	}

	return ((CGPValue *)GPV)->GetList();
}
