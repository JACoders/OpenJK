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

// Notes
// Make sure extension is stripped if it needs to be

// Template class must have
// 1. A GetName() accessor - a null terminated string case insensitive
// 2. A Destroy() function - normally "delete this"
// 3. SetNext(T *) and T *GetNext() functions

#define HASH_SIZE	1024

template <class T, int TSize = HASH_SIZE, int (*TCompare)(const char *, const char *) = &strcmp>

class CHash
{
private:
	T		*mHashTable[TSize];
	T		*mNext;
	int		mCount;
	T		*mPrevious;					// Internal work variable
	long	mHash;						// Internal work variable

	// Creates the hash value and sets the mHash member
	void CreateHash(const char *key)
	{
		int		i = 0;
		char	letter;

		mHash = 0;
		letter = *key++;
		while (letter)
		{
			mHash += (long)(letter) * (i + 119);

			i++;
			letter = *key++;
		}
		mHash &= TSize - 1;
	}
public:
	// Constructor
	CHash(void)
	{
		memset(mHashTable, 0, sizeof(mHashTable));
		mNext = NULL;
		mCount = 0;
		mPrevious = NULL;
		mHash = 0;
	}
	// Destructor
	~CHash(void)
	{
#ifdef _DEBUG
//		Com_OPrintf("Shutting down %s hash table .....", typeid(T).name());
#endif
		clear();
#ifdef _DEBUG
//		Com_OPrintf(" done\n");
#endif
	}
	// Returns the total number of entries in the hash table
	int count(void) const { return(mCount); }

	// Inserts an item into the hash table
	void insert(T *item)
	{
		CreateHash(item->GetName());
		item->SetNext(mHashTable[mHash]);
		mHashTable[mHash] = item;
		mCount++;
	}
	// Finds an item in the hash table (sets the mPrevious member)
	T *find(const char *key)
	{
		CreateHash(key);
		T *item = mHashTable[mHash];
		mPrevious = NULL;
		while(item)
		{
			mNext = item->GetNext();
			if(!TCompare(item->GetName(), key))
			{
				return(item);
			}
			mPrevious = item;
			item = mNext;
		}
		return(NULL);
	}
	// Remove item from the hash table referenced by key
	bool remove(const char *key)
	{
		T *item = find(key);
		if(item)
		{
			T *next = item->GetNext();
			if(mPrevious)
			{
				mPrevious->SetNext(next);
			}
			else
			{
				mHashTable[mHash] = next;
			}
			item->Destroy();
			mCount--;
			return(true);
		}
		return(false);
	}
	// Remove item from hash referenced by item
	bool remove(T *item)
	{
		return(remove(item->GetName()));
	}
	// Returns the first valid entry
	T *head(void)
	{
		mHash = -1;
		mNext = NULL;
		return(next());
	}
	// Returns the next entry in the hash table
	T *next(void)
	{
		T *item;

		assert(mHash < TSize);

		if(mNext)
		{
			item = mNext;
			mNext = item->GetNext();
			return(item);
		}
		mHash++;

		for( ; mHash < TSize; mHash++)
		{
			item = mHashTable[mHash];
			if(item)
			{
				mNext = item->GetNext();
				return(item);
			}
		}
		return(NULL);
	}
	// Destroy all entries in the hash table
	void clear(void)
	{
		T *item = head();
		while(item)
		{
			remove(item);
			item = next();
		}
	}
	// Override the [] operator
	T *operator[](const char *key) { return(find(key)); }
};

// end