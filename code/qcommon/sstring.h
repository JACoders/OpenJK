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

// Filename:-	sstring.h
//
// Gil's string template, used to replace Microsoft's <string> vrsion which doesn't compile under certain stl map<>
//	conditions...


#ifndef SSTRING_H
#define SSTRING_H

#include "../qcommon/q_shared.h"

template<int MaxSize>
class sstring
{
	struct SStorage
	{
		char data[MaxSize];
	};
	SStorage mStorage;
public:
/* don't figure we need this
	template<int oMaxSize>
	sstring(const sstring<oMaxSize> &o)
	{
		assert(strlen(o.mStorage.data)<MaxSize);
		strcpy(mStorage.data,o.mStorage.data);
	}
*/	
	sstring(const sstring<MaxSize> &o)
	{
		//strcpy(mStorage.data,o.mStorage.data);
		Q_strncpyz(mStorage.data,o.mStorage.data,sizeof(mStorage.data),qtrue);
	}
	sstring(const char *s)
	{
		//assert(strlen(s)<MaxSize);
		//strcpy(mStorage.data,s);
		Q_strncpyz(mStorage.data,s,sizeof(mStorage.data),qtrue);
	}
	sstring()
	{
		mStorage.data[0]=0;
	}
/* don't figure we need this
	template<int oMaxSize>
	sstring<oMaxSize> & operator =(const sstring<oMaxSize> &o)
	{
		assert(strlen(o.mStorage.data)<MaxSize);
		strcpy(mStorage.data,o.mStorage.data);
		return *this;
	}
*/	
	sstring<MaxSize> & operator=(const sstring<MaxSize> &o)
	{
		//strcpy(mStorage.data,o.mStorage.data);
		Q_strncpyz(mStorage.data,o.mStorage.data,sizeof(mStorage.data),qtrue);
		return *this;
	}
	sstring<MaxSize> & operator=(const char *s)
	{
		assert(strlen(s)<MaxSize);
		//strcpy(mStorage.data,s);
		Q_strncpyz(mStorage.data,s,sizeof(mStorage.data),qtrue);
		return *this;
	}
	char *c_str()
	{
		return mStorage.data;
	}	
	const char *c_str() const
	{
		return mStorage.data;
	}	
	int capacity() const
	{
		return MaxSize;	// not sure if this should be MaxSize-1? depends if talking bytes or strlen space I guess
	}
	int length() const
	{
		return strlen(mStorage.data);
	}
	bool operator==(const sstring<MaxSize> &o) const
	{
		if (!Q_stricmp(mStorage.data,o.mStorage.data))
		{
			return true;
		}
		return false;
	}
	bool operator!=(const sstring<MaxSize> &o) const
	{
		if (Q_stricmp(mStorage.data,o.mStorage.data)!=0)
		{
			return true;
		}
		return false;
	}
	bool operator<(const sstring<MaxSize> &o) const
	{
		if (Q_stricmp(mStorage.data,o.mStorage.data)<0)
		{
			return true;
		}
		return false;
	}
	bool operator>(const sstring<MaxSize> &o) const
	{
		if (Q_stricmp(mStorage.data,o.mStorage.data)>0)
		{
			return true;
		}
		return false;
	}
};

typedef sstring<MAX_QPATH> sstring_t;

#endif	// #ifndef SSTRING_H

/////////////////// eof ////////////////////

