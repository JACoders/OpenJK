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
// Scheduler
// ---------
// The scheduler is a common piece of game functionality.  To use it, simply add events
// at the given time, and call update() with the current time as frequently as you wish.
//
// Your event class MUST define a Fire() function which accepts a TCALLBACKPARAMS
// parameter.
//
// NOTES:
// 
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_SCHEDULER_VS_INC)
#define RATL_SCHEDULER_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
#if !defined(RATL_POOL_VS_INC)
	#include "pool_vs.h"
#endif
#if !defined(RATL_HEAP_VS_INC)
	#include "heap_vs.h"
#endif
namespace ratl
{


////////////////////////////////////////////////////////////////////////////////////////
// The Scheduler Class
////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class scheduler_base : public ratl_base
{
public:
	typedef typename T TStorageTraits;
	typedef typename T::TValue TTValue;
    ////////////////////////////////////////////////////////////////////////////////////
	// Capacity Enum
    ////////////////////////////////////////////////////////////////////////////////////
	static const int CAPACITY		= T::CAPACITY;
private:
	////////////////////////////////////////////////////////////////////////////////////
	// The Timed Event Class
	//
	// This class stores two numbers, a timer and an iterator to the events list.  We
	// don't store the event directly in the heap to make the swap operation in the
	// heap faster.  We define a less than operator so we can sort in the heap.
	//
    ////////////////////////////////////////////////////////////////////////////////////
	struct timed_event
	{
		float	mTime;
		int		mEvent;

		timed_event() {}
		timed_event(float time, int event) : mTime(time), mEvent(event)	{}
		bool	operator<  (const timed_event& t) const
		{
			return	(mTime > t.mTime);
		}
	};

	pool_base<TStorageTraits>			mEvents;
	heap_vs<timed_event, CAPACITY>		mHeap;

public:
	////////////////////////////////////////////////////////////////////////////////////
	// How Many Objects Are In This List
    ////////////////////////////////////////////////////////////////////////////////////
	int			size() const
	{
		// warning during a fire call, there will be one extra event
		return mEvents.size();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Are There Any Objects In This List?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		empty() const
	{
		return !size();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Is This List Filled?
    ////////////////////////////////////////////////////////////////////////////////////
	bool		full() const
	{
		return mEvents.full();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Clear All Elements
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		mEvents.clear();
		mHeap.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add An Event
    ////////////////////////////////////////////////////////////////////////////////////
	void		add(float time, const TTValue& e)
	{
		int	nLoc = mEvents.alloc(e);
		mHeap.push(timed_event(time, nLoc));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add An Event
    ////////////////////////////////////////////////////////////////////////////////////
	TTValue &	add(float time)
	{
		int	nLoc = mEvents.alloc();
		mHeap.push(timed_event(time, nLoc));
		return mEvents[nLoc];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Add A Raw Event for placement new
    ////////////////////////////////////////////////////////////////////////////////////
	TRatlNew *	add_raw(float time)
	{
		TRatlNew *ret = mEvents.alloc_raw();
		mHeap.push(timed_event(time, mEvents.pointer_to_index(ret)));
		return ret;
	}

	template<class TCALLBACKPARAMS>
	void		update(float time, TCALLBACKPARAMS& Params)
	{
		while (!mHeap.empty())
		{
			timed_event	Next = mHeap.top();
			if (Next.mTime>=time)
			{
				break;
			}
			mHeap.pop();
			mEvents[Next.mEvent].Fire(Params);
			mEvents.free(Next.mEvent);
		}
	}

	void update(float time)
	{
		while (!mHeap.empty())
		{
			timed_event	Next = mHeap.top();
			if (Next.mTime>=time)
			{
				break;
			}
			mHeap.pop();
			mEvents[Next.mEvent].Fire();
			mEvents.free(Next.mEvent);
		}
	}	
};


template<class T, int ARG_CAPACITY>
class scheduler_vs : public scheduler_base<storage::value_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::value_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	scheduler_vs() {}
};

template<class T, int ARG_CAPACITY>
class scheduler_os : public scheduler_base<storage::object_semantics<T,ARG_CAPACITY> >
{
public:
	typedef typename storage::object_semantics<T,ARG_CAPACITY> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	scheduler_os() {}
};

template<class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
class scheduler_is : public scheduler_base<storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> >
{
public:
	typedef typename storage::virtual_semantics<T,ARG_CAPACITY,ARG_MAX_CLASS_SIZE> TStorageTraits;
	typedef typename TStorageTraits::TValue TTValue;
	static const int CAPACITY		= ARG_CAPACITY;
	static const int MAX_CLASS_SIZE	= ARG_MAX_CLASS_SIZE;
	scheduler_is() {}
};

}
#endif
