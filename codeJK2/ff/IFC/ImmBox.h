/**********************************************************************
	Copyright (c) 1997 - 2000 Immersion Corporation

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

  FILE:		ImmBox.h

  PURPOSE:	Box Class for Immersion Foundation Classes

  STARTED:	11/04/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 11/15/99 sdr (Steve Rank): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMBOX_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMBOX_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmBaseTypes.h"
#include "ImmEffect.h"
#include "ImmEnclosure.h"



//================================================================
// Constants
//================================================================

const POINT IMM_BOX_MOUSE_POS_AT_START = { MAXLONG, MAXLONG };

#define     IMM_BOX_DEFAULT_STIFFNESS      5000
#define     IMM_BOX_DEFAULT_WIDTH          10
#define     IMM_BOX_DEFAULT_HEIGHT         IMM_ENCLOSURE_HEIGHT_AUTO
#define     IMM_BOX_DEFAULT_WALL_WIDTH     IMM_ENCLOSURE_WALL_WIDTH_AUTO

#define     IMM_BOX_DEFAULT_CENTER_POINT   IMM_BOX_MOUSE_POS_AT_START




//================================================================
// CImmBox
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmBox : public CImmEnclosure
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmBox();

	// Destructor
	virtual
	~CImmBox();


    //
    // ATTRIBUTES
    //

    public:


    BOOL
    ChangeParameters( 
        POINT pntCenter,
        LONG lStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwHeight = IMM_EFFECT_DONT_CHANGE,
        DWORD dwWallWidth = IMM_EFFECT_DONT_CHANGE,
        CImmEffect* pInsideEffect = (CImmEffect*) IMM_EFFECT_DONT_CHANGE  
        );

    BOOL
	ChangeParameters( 
        LPCRECT pRectOutside,
        LONG lStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwWallWidth = IMM_EFFECT_DONT_CHANGE,
        CImmEffect* pInsideEffect = (CImmEffect*) IMM_EFFECT_DONT_CHANGE  
        );


    //
    // OPERATIONS
    //

    public:


    BOOL 
	Initialize( 
        CImmDevice* pDevice, 
        DWORD dwWidth = IMM_ENCLOSURE_DEFAULT_WIDTH,
        DWORD dwHeight = IMM_ENCLOSURE_DEFAULT_HEIGHT,
        LONG lStiffness = IMM_BOX_DEFAULT_STIFFNESS,
        DWORD dwWallWidth = IMM_BOX_DEFAULT_WALL_WIDTH,
        POINT pntCenter = IMM_BOX_DEFAULT_CENTER_POINT,
        CImmEffect* pInsideEffect = NULL,
		DWORD dwNoDownload = 0  
        );


    BOOL 
	Initialize( 
        CImmDevice* pDevice, 
        LPCRECT pRectOutside,
        LONG lStiffness = IMM_BOX_DEFAULT_STIFFNESS,
        DWORD dwWallWidth = IMM_BOX_DEFAULT_WALL_WIDTH,
        CImmEffect* pInsideEffect = NULL,
		DWORD dwNoDownload = 0  
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

};


#endif // !defined(AFX_IMMBOX_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
