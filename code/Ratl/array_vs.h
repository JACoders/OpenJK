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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD TEMPLATE LIBRARY
//  (c) 2002 Activision
//
//
// Array
// -----
// This array class is little more than an assert loaded wrapper around a standard
// array.
//
//
//
// NOTES:
// 
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_ARRAY_VS)
#define RATL_ARRAY_VS


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif

namespace ratl
{

template<class T, int ARG_CAPACITY>
class array_vs : public array_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	array_vs() {}
};

template<class T, int ARG_CAPACITY>
class array_os : public array_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	array_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class array_is : public array_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	array_is() {}
};

}
#endif
