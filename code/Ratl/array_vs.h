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
 	enum 
	{
		CAPACITY		= ARG_CAPACITY
	};
	array_vs() {}
};

template<class T, int ARG_CAPACITY>
class array_os : public array_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
 	enum 
	{
		CAPACITY		= ARG_CAPACITY
	};
	array_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class array_is : public array_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
 	enum 
	{
		CAPACITY		= ARG_CAPACITY,
		MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE
	};
	array_is() {}
};

}
#endif