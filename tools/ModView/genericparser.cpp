// Main Parsing Class
//

#if !defined(GENERICPARSER_H_INC)
	#include "GenericParser.h"
#endif



#if 0
//
//
// CParseGroup
//
//


/************************************************************************************************
 * CParseGroup
 *    copy constructor. recursively copies all subgroups and pairs.
 *
 * Input
 *    an existing parse group
 *
 * Output
 *    none
 *
 ************************************************************************************************/
CParseGroup::CParseGroup(CParseGroup *orig)
{
	CParseGroup		*newGroup = 0;

	if (orig == 0)
	{
		return;
	}
	mWriteable = orig->mWriteable;
	mName = orig->mName;
	mParent = orig->mParent;
	for (TParsePairIter itp = orig->PairsBegin(); itp != orig->PairsEnd(); itp++)
	{
		AddPair((*itp).first, (*itp).second);
	}
	for (TParseGroupIter itg = orig->GroupsBegin(); itg != orig->GroupsEnd(); itg++)
	{
		newGroup = new CParseGroup((*itg).second);
		AddSubGroup(newGroup);
	}
}


/************************************************************************************************
 * ~CParseGroup
 *	basic destructor. deallocate any sub groups before going away
 *
 * inputs:
 *	none
 *
 * return:
 *	nothing
 ************************************************************************************************/
CParseGroup::~CParseGroup()
{
	Clean();
}


/************************************************************************************************
 * FindPairValue
 *	Return the value assigned to the provided key. If said key doesn't exist for this
 * ParseGroup, the provided default value is returned.
 *
 * inputs:
 *	a key string to be used as an element in CParseGroup::mPairs
 *	a default value string to be returned in the event that the provided key isn't found
 *
 * return:
 *	either the string value stored at CParseGroup::mPairs[key] or, failing that, the provided
 * default value
 *
 ************************************************************************************************/
string CParseGroup::FindPairValue(string key, string defaultVal)
{
	map<string, string>::iterator	it;

	it = mPairs.find(key);
	if (it != mPairs.end())
	{
		return (*it).second;
	}

	return defaultVal;
}

/************************************************************************************************
 * FindSubGroup
 *    search for a subgroup with a particular name
 *
 * Input
 *    name of subgroup, boolean indicating whether or not to search recursively
 *
 * Output
 *    pointer to the desired subgroup, NULL if not found
 *
 ************************************************************************************************/
CParseGroup	*CParseGroup::FindSubGroup(string name, bool recurse/*false*/)
{
	TParseGroupIter	it = mSubGroups.find(name);
	CParseGroup		*subGroup = 0, *findGroup = 0;

	if (it != mSubGroups.end())
	{
		return (*it).second;
	}

	// didn't find it in the top level of groups. if we're supposed to recurse, search
	//our top level's subgroups, else return NULL
	if (recurse)
	{
		for (it = mSubGroups.begin(); it != mSubGroups.end(); it++)
		{
			subGroup = (*it).second;
			if (subGroup)
			{
				findGroup = subGroup->FindSubGroup(name, recurse);
				if (findGroup)
				{
					return findGroup;
				}
			}
		}
	}
	return NULL;
}

/************************************************************************************************
 * FindSubGroupWithPair
 *	finds a subgroup that contains the provided key/value pair
 *
 * inputs:
 *	strings key and value for locating the subgroup
 *
 * return:
 *	pointer to the ParseGroup  representing the subgroup we're after or NULL if we didn't find it
 ************************************************************************************************/
CParseGroup *CParseGroup::FindSubGroupWithPair(string key, string value)
{
	TParseGroupIter	it;
	TParsePairIter	itp;

	for (it = mSubGroups.begin(); it != mSubGroups.end(); it++)
	{
		itp = ((CParseGroup*)(*it).second)->mPairs.find(key);
		if (itp != ((CParseGroup*)(*it).second)->mPairs.end())
		{
			if (value.compare((*itp).second) == 0)
			{
				// found a subgroup with a key/value pair matching the one we're looking for
				return ((CParseGroup*)(*it).second);
			}
		}
	}
	// none of our subgroups contains the key/value pair
	return NULL;
}


/************************************************************************************************
 * DeleteSubGroup
 *    find the given subgroup in our multimap of subgroups and remove it. also, try to find
 * it in our vector of ordered subgroups and remove it from there.
 *
 * Input
 *    parse group to be deleted
 *
 * Output
 *    true if it was found and deleted, false otherwise
 *
 ************************************************************************************************/
bool CParseGroup::DeleteSubGroup(CParseGroup *delGroup)
{
	TParseGroupIter itg;

	if (delGroup == 0)
	{
		return false;
	}
	itg = mSubGroups.find(delGroup->GetName());
	
	if (itg != mSubGroups.end())
	{
		// this is a multimap, so make sure our pointers match
		while ( ((*itg).second != delGroup) && (itg != mSubGroups.end()) )
		{
			itg++;
		}
		if ((*itg).second == delGroup)
		{
			// try to find it in mInOrderSubGroups
			if (mInOrderSubGroups.size())
			{
				vector<pair<string, CParseGroup*> >::iterator itv = mInOrderSubGroups.begin();
				for (int i = 0; i < mInOrderSubGroups.size(); i++, itv++)
				{
					if (mInOrderSubGroups[i].second == delGroup)
					{
						mInOrderSubGroups.erase(itv);
						break;
					}
				}
			}
			delete delGroup;
			delGroup = 0;
			mSubGroups.erase(itg);
			return true;
		}
	}
	return false;
}


/************************************************************************************************
 * Clean
 *	cleans up our sub group list
 *
 * inputs:
 *	none
 *
 * return:
 *	nothing
 ************************************************************************************************/
void CParseGroup::Clean()
{
	TParseGroupIter	it;

	for (it = mSubGroups.begin(); it != mSubGroups.end(); it++)
	{
		delete (CParseGroup*)(*it).second;
	}
	mSubGroups.clear();

	mInOrderPairs.clear();
	mInOrderSubGroups.clear();
}



/************************************************************************************************
 * WriteLine
 *	writes a line of data to the provided output string
 *
 * inputs:
 *	a brace depth, allowing pretty formatted text, a line of text to be written, and an output string
 *
 * return:
 *	true if data is written successfully, false otherwise
 ************************************************************************************************/
bool CParseGroup::WriteLine(int depth, const string line, string &output)
{
	for (int i = 0; i < depth; i++)
	{
		output += "\t";
	}
	output += line;
	output += "\n";
	return true;
}


/************************************************************************************************
 * WriteGroup
 *	write this group's pairs and then its subgroups 
 *
 * inputs:
 *	name of the object, indentation depth, the destination string
 *
 * return:
 *	true if data is written successfully, false otherwise
 ************************************************************************************************/
bool CParseGroup::WriteGroup(string name, int depth, string &output)
{
	string		curLine;
	CParseGroup	*subGroup = 0;
	vector< pair<string, string> >		&inOrderPairs = GetInOrderPairs();
	vector< pair<string, CParseGroup*> > &inOrderGroups = GetInOrderGroups();

	// first, write out the name (e.g. "hook") of the object
	WriteLine(depth, name, output);

	// now write out an open brace to begin the innards of our object
	WriteLine(depth, "{", output);

	// write out the key/value pairs
	for (int p = 0; p < inOrderPairs.size(); p++)
	{
		curLine = "\"";
		curLine += inOrderPairs[p].first;
		curLine += "\"\t\"";
		curLine += inOrderPairs[p].second;
		curLine += "\"";
		WriteLine(depth+1, curLine, output);
	}

	// recursively write out the subgroups
	for (int g = 0; g < inOrderGroups.size(); g++)
	{
		WriteLine(depth+1, "", output);
		subGroup = inOrderGroups[g].second;
		if (subGroup)
		{
			subGroup->WriteGroup(inOrderGroups[g].first, depth+1, output);
		}
	}

	// now write out a close brace
	WriteLine(depth, "}", output);

	return true;
}


//
//
// CGenericParser
//
//


/************************************************************************************************
 * CGenericParser
 *	constructor. inits the max token length.
 *
 * input:
 *	none
 *
 * return:
 *	nothing
 ************************************************************************************************/
CGenericParser::CGenericParser():
mMaxTokenLength(1024)
{
	mLineCount = 0;
	mBraceDepth = 0;
	mCurGroup = NULL;
}


/************************************************************************************************
 * ~CGenericParser
 *	deallocates any CParseGroups we created, general clean up
 *
 * inputs:
 *	none
 *
 * return:
 *	nothing
 ************************************************************************************************/
CGenericParser::~CGenericParser()
{
	mGroups.Clean();
}

/************************************************************************************************
 * SkipWhiteSpace
 *	loops through the char's in data and points data to the next non-whitespace character. sets
 *	newLine variable to true if one of the whitespace char's is a new line.
 *
 * input:
 *	pointer to text data, pointer to a boolean which states whether or not a newline is encountered
 *
 * return:
 *	the altered data pointer if it's pointing to a valid char. returns NULL if we reached the
 *	end of the data.
 ************************************************************************************************/
char *CGenericParser::SkipWhiteSpace( char *data, bool *newLine )
{
	char ch;

	while(isspace(ch = *data))
	{
		if( '\n' == ch )
		{
			*newLine = true;
			mLineCount++;
		}
		data++;
	}
	if (0 == ch)
	{
		// reached the end of our data
		return NULL;
	}
	return data;
}


/************************************************************************************************
 * EatCurrentLine
 *	increments the data pointer until it hits a new line or we run out of char data
 *
 * input:
 *	pointer to text data
 *
 * return:
 *	nothing
 ************************************************************************************************/
void CGenericParser::EatCurrentLine( char **data )
{
	while (**data && **data != '\n')
	{
		if ('{' == **data)
		{
			mBraceDepth++;
		}
		else if ('}' == **data)
		{
			if (0 == mBraceDepth)
			{
				// too many closing braces
			}
			else
			{
				mBraceDepth--;
			}
		}
		(*data)++;
	}
}


/************************************************************************************************
 * GetToken
 *	parses a text string to procure the next viable (non-whitespace, non-comment) token. 
 *	said token will be stored in CGenericParser::mToken
 *
 * input:
 *	pointer to character data to be parsed, boolean stating whether or not new lines are 
 *	acceptable characters for the upcoming token
 *
 * return:
 *	reference to the string containing the text value of the new token
 *
 ************************************************************************************************/
string &CGenericParser::GetToken( char **dataPtr, const bool allowNewLines )
{
	char	ch = 0;
	int		len = 0;
	bool	newLine = false;
	char	*data = *dataPtr;

	// reset our current token
	mToken = "";

	// return an empty string if text data is NULL
	if ( !data )
	{
		*dataPtr = NULL;
		return mToken;
	}

	// loop through the text data, skip whitespace, skip comments, get to the 
	//beginning of the next token
	while ( data = SkipWhiteSpace( data, &newLine ) )
	{
		if ( newLine && !allowNewLines )
		{
			*dataPtr = data;
			return mToken;
		}

		ch = *data;

		// skip // comments
		if ( ('/' == ch) && ('/' == data[1]) )
		{
			EatCurrentLine(&data);
		}
		// skip /* */ comments
		else if ( ('/' == ch) && ('*' == data[1]) ) 
		{
			while ( *data && ((*data != '*') || (data[1] != '/')) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}
	// if SkipWhiteSpace returned NULL, we got to the end of our data without finding
	//another valid token
	if ( !data )
	{
		*dataPtr = NULL;
		return mToken;
	}

	//
	// at this point we're pointing at the next valid char in our data stream
	//

	// handle quoted strings
	if ('\"' == ch)
	{
		data++;
		while (1)
		{
			ch = *data++;
			if (('\"' == ch) || !ch)
			{
				*dataPtr = (char*)data;
				return mToken;
			}
			if (len < mMaxTokenLength)
			{
				mToken.append(1,ch);
				len++;
			}
		}
	}

	// ok, these next characters are a token
	do
	{
		if ('{' == *data)
		{
			mBraceDepth++;
		}
		else if ('}' == *data)
		{
			if (0 == mBraceDepth)
			{
				// too many closing braces
			}
			else
			{
				mBraceDepth--;
			}
		}
		if (len < mMaxTokenLength)
		{
			mToken.append(1,ch);
			len++;
		}
		data++;
		ch = *data;
		if ( '\n' == ch )
		{
			mLineCount++;
		}
	} while (!isspace(ch));

	if (len == mMaxTokenLength)
	{
		// token is too long...excess characters discarded
		len = 0;
	}

	*dataPtr = ( char * ) data;
	return mToken;
}


/************************************************************************************************
 * ReturnToBraceDepthZero
 *	this function is called when a parse error is encountered. its goal is to eat any and all
 *	text until it returns to brace depth zero, where the parser can hopefully start over
 *
 * inputs:
 *	pointer to character data to be parsed
 *
 * return:
 *	nothing
 ************************************************************************************************/
void CGenericParser::ReturnToBraceDepthZero(char **dataPtr)
{
	// we already parsed a mismatched '}' so discount it in terms of subtracting from brace depth
	mBraceDepth++;

	// commence the eating of tokens
	while (1)
	{
		GetToken(dataPtr);
		if ( (0 == mBraceDepth) || (mToken.empty()) )
		{
			return;
		}
	}
}


/************************************************************************************************
 * ParseGroup
 *	recursively parses a text string using a nested-block format. basically parses a pair of
 *	tokens at once, if possible.
 *
 * inputs:
 *	pointer to character data to be parsed
 *
 * return:
 *	true if a group is parsed, false if end-of-file
 ************************************************************************************************/
bool CGenericParser::ParseGroup(char **dataPtr)
{
	string tok1;
	string tok2;

	while (1)
	{
		tok1 = GetToken(dataPtr);
		if (tok1.empty())
		{
			// reached end of data
			return false;
		}
		if ('}' == tok1[0])
		{
			// ending a ParseGroup
			mCurGroup = mCurGroup->GetParent();
			return true;
		}
		tok2 = GetToken(dataPtr);
		if (tok2.empty())
		{
			// empty group or a key-value pair with no value, just a key
		}
		else if ('{' == tok2[0])
		{
			// we're starting a new CParseGroup and its name is currently in tok1
			CParseGroup		*newGroup = new CParseGroup(tok1, mCurGroup, mWriteable);
			if (NULL == mCurGroup)
			{
				// this is a top level group so add it to the parser's list 
				mGroups.AddSubGroup(newGroup);
				mCurGroup = newGroup;
			}
			else
			{
				// this is somebody's subgroup
				mCurGroup->AddSubGroup(newGroup);
				// time for some ol' fashioned, down-home recursion. yeehaw!
				mCurGroup = newGroup;
				ParseGroup(dataPtr);
			}
		}
		else if ('}' == tok2[0])
		{
			// error. try to recover.
			ReturnToBraceDepthZero(dataPtr);
		}
		else
		{
			if (mCurGroup == 0)
			{
				// assume we're adding a key/value pair at the file level
				mGroups.AddPair(tok1, tok2);
			}
			else
			{
				// we're considering tok1 as a key and tok2 as a value
				mCurGroup->AddPair(tok1, tok2);
			}
		}
	}
}

/************************************************************************************************
 * Parse
 *	parses text data for CParseGroup's until end-of-file is reached
 *
 * inputs:
 *	pointer to character data to be parsed, boolean indicating if we should wipe our
 * stored data before parsing more, boolean indicating if we should make this data
 * writeable
 *
 * return:
 *	nothing
 ************************************************************************************************/
bool CGenericParser::Parse(char **dataPtr, bool cleanFirst/*true*/, bool writeable /*false*/)
{
	// a quick test to find hand-edit errors that always seem to manifest as perplexing bugs before we find them...
	{ 
		string strBraceTest(*dataPtr);
		int iOpeningBraces = 0;
		int iClosingBraces = 0;

		char *p;
		
		while ((p=strchr(strBraceTest.c_str(),'{'))!=NULL)
		{
			*p = '#';	// anything that's not a '{'
			iOpeningBraces++;
		}

		while ((p=strchr(strBraceTest.c_str(),'}'))!=NULL)
		{
			*p = '#';	// anything that's not a '}'
			iClosingBraces++;
		}

		if (iOpeningBraces != iClosingBraces)
		{
			// maybe print something here, but in any case...
			//
			return false;
		}
	}

	if (cleanFirst)
	{
		// if this is a new stream of data, init some stuff
		mGroups.Clean();
	}

	mWriteable = writeable;
	mGroups.SetWriteable(mWriteable);

	while (ParseGroup(dataPtr))
	{
		;
	}

	return true;
}


/************************************************************************************************
 * Write
 *    take all of the stuff we have stored as ParseGroups and write it in an aesthetically
 * pleasing form to an output string
 *
 * Input
 *    output string
 *
 * Output
 *    true if every parsegroup wrote without incident, false otherwise
 *
 ************************************************************************************************/
bool CGenericParser::Write(string &output)
{
	string		curLine;
	CParseGroup *subGroup = 0;
	vector< pair<string, string> >		&inOrderPairs = mGroups.GetInOrderPairs();
	vector< pair<string, CParseGroup*> > &inOrderGroups = mGroups.GetInOrderGroups();

	if (!mWriteable)
	{
		return false;
	}
	// write out the key/value pairs
	for (int p = 0; p < inOrderPairs.size(); p++)
	{
		curLine = "\"";
		curLine += inOrderPairs[p].first;
		curLine += "\"\t\"";
		curLine += inOrderPairs[p].second;
		curLine += "\"";
		mGroups.WriteLine(0, curLine, output);
	}

	// recursively write out the subgroups
	for (int g = 0; g < inOrderGroups.size(); g++)
	{
		mGroups.WriteLine(0, "", output);
		subGroup = inOrderGroups[g].second;
		if (subGroup)
		{
			subGroup->WriteGroup(inOrderGroups[g].first, 0, output);
		}
	}

	return true;
}


/************************************************************************************************
 * AddParseGroup
 *    if you want to be able to write out a parsegroup hierarchy, you may want to create
 * some of it yourself rather than just rely on what you've read in. that's what this
 * fn is for. this is returning a pointer to an object allocate on the heap, so make sure
 * it gets deallocated (if you add it into the existing hierarchy of a CGenericParser that
 * will take care of it for you)
 *
 * Input
 *    name of the new parse group, parent of your new parse group (can't be NULL), whether
 * or not your new parse group will be writeable (defaults to true)
 *
 * Output
 *    pointer to new parse group
 *
 ************************************************************************************************/
CParseGroup *CGenericParser::AddParseGroup(string groupName, CParseGroup &groupParent, bool writeable /*true*/)
{
	CParseGroup *newGroup =  new CParseGroup(groupName, &groupParent, writeable);

	groupParent.AddSubGroup(newGroup);
	return newGroup;
}


/************************************************************************************************
 * DeleteParseGroup
 *    recursively deletes a parse group
 *
 * Input
 *    parent of group to be deleted, pointer to parse group to be deleted
 *
 * Output
 *    true if it was found and deleted
 *
 ************************************************************************************************/
bool CGenericParser::DeleteParseGroup(CParseGroup *parentGroup, CParseGroup *delGroup)
{
	return parentGroup->DeleteSubGroup(delGroup);
}


/************************************************************************************************
 * <function>
 *
 * inputs:
 *
 * return:
 *
 ************************************************************************************************/
#endif
