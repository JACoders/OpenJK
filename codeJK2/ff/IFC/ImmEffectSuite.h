
/**********************************************************************
	Copyright (c) 1999 - 2000 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		801 Fox Lane
		San Jose, CA 95131
		408-467-1900

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL IMMERSION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  FILE:		ImmEffectSuite.h

  PURPOSE:	Caching of effects

  STARTED:	6/16/99 Jeff Mallett

  NOTES/REVISIONS:

**********************************************************************/

#if !defined(AFX_FEELEFFECTSUITE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELEFFECTSUITE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmBaseTypes.h"

#ifdef IFC_EFFECT_CACHING

class CImmDevice; //#include "ImmDevice.h"
class CImmEffect; //#include "ImmEffect.h"

typedef enum {
	IMMCACHE_NOT_ON_DEVICE,
	IMMCACHE_ON_DEVICE,
	IMMCACHE_SWAPPED_OUT
} ECacheState;


//================================================================
// CEffectList, CEffectListElement
//================================================================

class DLLIFC CEffectListElement
{
public:
	CEffectListElement() : m_pImmEffect(NULL), m_pNext(NULL) { }

	CImmEffect *m_pImmEffect;
	CEffectListElement *m_pNext;
};

class DLLIFC CEffectList
{
public:
	CEffectList() : m_pFirstEffect(NULL) { }
	~CEffectList();
	BOOL AddEffect(CImmEffect *pImmEffect);
	BOOL RemoveEffect(const CImmEffect *pImmEffect);
	void ClearDevice(CImmDevice *pImmDevice);

	CEffectListElement *m_pFirstEffect;
};


//================================================================
// CImmEffectSuite
//================================================================

class CImmEffectSuite
{
public:
	CImmEffectSuite() : m_bCurrentSuite(false) { }
	CEffectListElement *GetFirstEffect();
	void AddEffect(CImmEffect *pImmEffect);
	void RemoveEffect(CImmEffect *pImmEffect);
	void SetPriorities(short priority);

	BOOL m_bCurrentSuite; // Is the suite the "current suite"?
private:
	CEffectList m_EffectList; // List of effects in suite
};

#endif // IFC_EFFECT_CACHING
#endif // !defined(AFX_FEELEFFECTSUITE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
