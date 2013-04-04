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

  FILE:		ImmProjects.h

  PURPOSE:	CImmProject
               Manages a set of forces in a project.
               There will be a project for each opened IFR file.
			CImmProjects
			   Manages a set of projects

  STARTED:	2/22/99 by Jeff Mallett


  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/

#ifndef	__IMM_PROJECTS_H
#define __IMM_PROJECTS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif


#include "IFCErrors.h"
#include "ImmBaseTypes.h"
#include "ImmDevice.h"
#include "ImmCompoundEffect.h"

class CImmProjects;


//================================================================
// CImmProject
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmProject
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	public:

	CImmProject();

	~CImmProject();

	void
	Close();


    //
    // ATTRIBUTES
    //

	public:

	CImmDevice*
	GetDevice() const
		{ return m_pDevice; }

	BOOL
	GetIsOpen() const
		{ return m_hProj != NULL; }

	CImmCompoundEffect *
	GetCreatedEffect(
		LPCSTR lpszEffectName
		);

	CImmCompoundEffect *
	GetCreatedEffect(
		int nIndex
		);

	int
	GetNumEffectsFromIFR();
	
	LPCSTR
	GetEffectNameFromIFRbyIndex(
		int nEffectIndex
		);

	LPCSTR
	GetEffectSoundPathFromIFR(
		LPCSTR lpszEffectName
		);

	DWORD 
	GetEffectType(
		LPCSTR lpszEffectName
		);

	DWORD
	GetEffectTypeFromIFR(
		LPCSTR lpszEffectName
		);

	DWORD
	GetEffectTypeFromIFR(
		int nEffectIndex
		);

	int
	GetNumCreatedEffects()
	{ return m_nCreatedEffects;} 

    //
    // OPERATIONS
    //

	public:

	BOOL
	Start(
		LPCSTR lpszEffectName = NULL, 
		DWORD dwIterations = IMM_EFFECT_DONT_CHANGE,
		DWORD dwFlags = 0,
		CImmDevice* pDevice = NULL
		);

	BOOL
	Stop(
		LPCSTR lpszEffectName = NULL
		);

	BOOL
	OpenFile(
		LPCSTR lpszFilePath,
		CImmDevice *pDevice
		);

	BOOL
	LoadProjectFromResource(
		HMODULE hRsrcModule,
		LPCSTR pRsrcName,
		CImmDevice *pDevice
		);

	BOOL
	LoadProjectFromMemory(
		LPVOID pProjectDef,
		CImmDevice *pDevice
		);

	BOOL
	LoadProjectObjectPointer(
		BYTE *pMem,
		CImmDevice *pDevice
		);

	BOOL
	WriteToFile(
		LPCSTR lpszFilename
		);

	CImmCompoundEffect *
	CreateEffect(
		LPCSTR lpszEffectName,
		CImmDevice* pDevice = NULL,
		DWORD dwNoDownload = 0
		);

	CImmCompoundEffect *
	CreateEffectByIndex(
		int nEffectIndex,
		CImmDevice* pDevice = NULL,
		DWORD dwNoDownload = 0
		);

	CImmCompoundEffect *
	AddEffect(
		LPCSTR lpszEffectName,
		GENERIC_EFFECT_PTR pObject
		);

#if (IFC_VERSION >= 0x0101)
	void
	DestroyEffect(
		CImmCompoundEffect *pCompoundEffect
		);
#endif

//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

	void
	set_next(
		CImmProject *pNext
		)
		{ m_pNext = pNext; }

	CImmProject *
	get_next() const
		{ return m_pNext; }

	void
	append_effect_to_list(
		CImmCompoundEffect* pEffect
		);
#if (IFC_VERSION >= 0x0101)
	BOOL
	remove_effect_from_list(
		CImmCompoundEffect* pEffect
		);
#endif

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
		CImmEffect::InitializeFromProject(
			CImmProject &project,
			LPCSTR lpszEffectName,
			CImmDevice* pDevice, /* = NULL */
			DWORD dwNoDownload // = 0
		);

#ifdef PROTECT_AGAINST_DELETION
	friend CImmCompoundEffect::~CImmCompoundEffect();
#endif

	friend class CImmProjects;

    //
    // INTERNAL DATA
    //

	protected:

	HIFRPROJECT m_hProj;
	DWORD m_dwProjectFileType;
	CImmCompoundEffect* m_pCreatedEffects;
	CImmDevice* m_pDevice;
	LPDIRECTINPUT m_piDI7;
	LPDIRECTINPUTDEVICE2 m_piDIDevice7;
	TCHAR m_szProjectFileName[MAX_PATH];

	int m_nCreatedEffects;

	private:

	CImmProject* m_pNext;
};



//================================================================
// CImmProjects
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmProjects
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	public:

	CImmProjects() : m_pProjects(NULL) { }

	~CImmProjects();

	void
	Close();


    //
    // ATTRIBUTES
    //

	public:

	CImmProject *
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
		CImmDevice *pDevice
		);

	long
	LoadProjectFromResource(
		HMODULE hRsrcModule,
		LPCSTR pRsrcName,
		CImmDevice *pDevice
		);

	long
	LoadProjectFromMemory(
		LPVOID pProjectDef,
		CImmDevice *pDevice
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

	CImmProject *m_pProjects;
};



#endif // __IMM_PROJECTS_H
