/**********************************************************************
	Copyright (c) 1999 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		2158 Paragon Drive
		San Jose, CA 95131
		408-467-1900

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL IMMERSION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  FILE:		FeelCompoundEffect.h

  PURPOSE:	Manages Compound Effects for Force Foundation Classes

  STARTED:	2/24/99 by Jeff Mallett

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(__FEELCOMPOUNDEFFECT_H)
#define __FEELCOMPOUNDEFFECT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include "FeelBaseTypes.h"
#include "FeelEffect.h"

#include "FeelitIFR.h"


//================================================================
// CFeelCompoundEffect
//================================================================
// Represents a compound effect, such as might be created in
// I-FORCE Studio.  Contains an array of effect objects.
// Methods iterate over component effects, passing the message
// to each one.
// Also, has stuff for being used by CFeelProject:
//   * next pointer so can be put on a linked list
//   * force name

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelCompoundEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	protected:
	// Constructs a CFeelCompoundEffect
	// Don't try to construct a CFeelCompoundEffect yourself.
    // Instead let CFeelProject construct it for you.
	CFeelCompoundEffect(
		IFREffect **hEffects,
		long nEffects
		);

	public:

	~CFeelCompoundEffect();


    //
    // ATTRIBUTES
    //

	public:

	long
	GetNumberOfContainedEffects() const
		{ return m_nEffects; }
    
	const char *
	GetName() const
		{ return m_lpszName; }

	GENERIC_EFFECT_PTR
	GetContainedEffect(
		long index
		);


	//
    // OPERATIONS
    //

	public:

	// Start all the contained effects
	BOOL Start(
		DWORD dwIterations = 1,
		DWORD dwFlags = 0
		);

	// Stop all the contained effects
	BOOL Stop();


//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

	BOOL initialize(
		CFeelDevice* pDevice,
		IFREffect **hEffects
		);

	BOOL
	set_contained_effect(
		GENERIC_EFFECT_PTR pObject,
		int index = 0
		);

	BOOL
	set_name(
		const char *lpszName
		);

	void
	set_next(
		CFeelCompoundEffect *pNext
		)
		{ m_pNext = pNext; }

	CFeelCompoundEffect *
	get_next() const
		{ return m_pNext; }


    //
    // FRIENDS
    //

	public:

	friend class CFeelProject;


    //
    // INTERNAL DATA
    //

	protected:

	GENERIC_EFFECT_PTR *m_paEffects; // Array of force class object pointers
	long m_nEffects; // Number of effects in m_paEffects

	private:

	char *m_lpszName; // Name of the compound effect
	CFeelCompoundEffect *m_pNext; // Next compound effect in the project
};

#endif
