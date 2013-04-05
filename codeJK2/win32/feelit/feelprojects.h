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

  FILE:		FeelProjects.h

  PURPOSE:	CFeelProject
               Manages a set of forces in a project.
               There will be a project for each opened IFR file.
			CFeelProjects
			   Manages a set of projects

  STARTED:	2/22/99 by Jeff Mallett


  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/

#ifndef	__FEEL_PROJECTS_H
#define __FEEL_PROJECTS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif


#include "FFCErrors.h"
#include "FeelBaseTypes.h"
#include "FeelDevice.h"
#include "FeelCompoundEffect.h"

class CFeelProjects;


//================================================================
// CFeelProject
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelProject
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	public:

	CFeelProject() :
			m_hProj(NULL), m_pCreatedEffects(NULL),
			m_pNext(NULL), m_pDevice(NULL)
		{ }

	~CFeelProject();

	void
	Close();


    //
    // ATTRIBUTES
    //

	public:

	CFeelDevice*
	GetDevice() const
		{ return m_pDevice; }

	BOOL
	GetIsOpen() const
		{ return m_hProj != NULL; }

	CFeelCompoundEffect *
	GetCreatedEffect(
		LPCSTR lpszEffectName
		);


    //
    // OPERATIONS
    //

	public:

	BOOL
	Start(
		LPCSTR lpszEffectName = NULL, 
		DWORD dwIterations = 1,
		DWORD dwFlags = 0,
		CFeelDevice* pDevice = NULL
		);

	BOOL
	Stop(
		LPCSTR lpszEffectName = NULL
		);

	BOOL
	OpenFile(
		LPCSTR lpszFilePath,
		CFeelDevice *pDevice
		);

	LoadProjectObjectPointer(
		BYTE *pMem,
		CFeelDevice *pDevice
		);

	CFeelCompoundEffect *
	CreateEffect(
		LPCSTR lpszEffectName,
		CFeelDevice* pDevice = NULL
		);

	CFeelCompoundEffect *
	CreateEffectByIndex(
		int nEffectIndex,
		CFeelDevice* pDevice = NULL
		);

	CFeelCompoundEffect *
	AddEffect(
		LPCSTR lpszEffectName,
		GENERIC_EFFECT_PTR pObject
		);


//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

	void
	set_next(
		CFeelProject *pNext
		)
		{ m_pNext = pNext; }

	CFeelProject *
	get_next() const
		{ return m_pNext; }

	void
	append_effect_to_list(
		CFeelCompoundEffect* pEffect
		);

	IFREffect **
	create_effect_structs(
		LPCSTR lpszEffectName,
		int &nEff
		);

	IFREffect **
	create_effect_structs_by_index(
		int nEffectIndex,
		int &nEff
		);

	BOOL
	release_effect_structs(
		IFREffect **hEffects
		);

    //
    // FRIENDS
    //

	public:

	friend BOOL 
		CFeelEffect::InitializeFromProject(
			CFeelProject &project,
			LPCSTR lpszEffectName,
			CFeelDevice* pDevice /* = NULL */
		);

	friend class CFeelProjects;

    //
    // INTERNAL DATA
    //

	protected:

	HIFRPROJECT m_hProj;
	CFeelCompoundEffect* m_pCreatedEffects;
	CFeelDevice* m_pDevice;

	private:

	CFeelProject* m_pNext;
};



//================================================================
// CFeelProjects
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelProjects
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	public:

	CFeelProjects() : m_pProjects(NULL) { }

	~CFeelProjects();

	void
	Close();


    //
    // ATTRIBUTES
    //

	public:

	CFeelProject *
	GetProject(
		int index = 0
		);


    //
    // OPERATIONS
    //

	public:

	BOOL
	Stop();

	long
	OpenFile(
		LPCSTR lpszFilePath,
		CFeelDevice *pDevice
		);


//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:


    //
    // INTERNAL DATA
    //
	protected:

	CFeelProject *m_pProjects;
};



#endif // __FEEL_PROJECTS_H
