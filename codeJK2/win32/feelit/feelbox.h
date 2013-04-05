/**********************************************************************
	Copyright (c) 1997 Immersion Corporation

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

  FILE:		FeelBox.h

  PURPOSE:	Box Class for Feelit API Foundation Classes

  STARTED:	11/04/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELBOX_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELBOX_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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
#include "FeelEnclosure.h"



//================================================================
// Constants
//================================================================

const POINT FEEL_BOX_MOUSE_POS_AT_START = { MAXLONG, MAXLONG };

#define     FEEL_BOX_DEFAULT_STIFFNESS      5000
#define     FEEL_BOX_DEFAULT_WIDTH          10
#define     FEEL_BOX_DEFAULT_HEIGHT         FEEL_ENCLOSURE_HEIGHT_AUTO
#define     FEEL_BOX_DEFAULT_WALL_WIDTH     FEEL_ENCLOSURE_WALL_WIDTH_AUTO

#define     FEEL_BOX_DEFAULT_CENTER_POINT   FEEL_BOX_MOUSE_POS_AT_START

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_BOX_MOUSE_POS_AT_START	FEEL_BOX_DEFAULT_STIFFNESS

#define     FORCE_BOX_DEFAULT_STIFFNESS		FEEL_BOX_DEFAULT_STIFFNESS
#define     FORCE_BOX_DEFAULT_WIDTH			FEEL_BOX_DEFAULT_WIDTH
#define     FORCE_BOX_DEFAULT_HEIGHT		FEEL_BOX_DEFAULT_HEIGHT
#define     FORCE_BOX_DEFAULT_WALL_WIDTH	FEEL_BOX_DEFAULT_WALL_WIDTH

#define     FORCE_BOX_DEFAULT_CENTER_POINT	FEEL_BOX_DEFAULT_CENTER_POINT



//================================================================
// CFeelBox
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelBox : public CFeelEnclosure
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CFeelBox();

	// Destructor
	virtual
	~CFeelBox();


    //
    // ATTRIBUTES
    //

    public:


    BOOL
    ChangeParameters( 
        POINT pntCenter,
        LONG lStiffness = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwWidth = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwHeight = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwWallWidth = FEEL_EFFECT_DONT_CHANGE,
        CFeelEffect* pInsideEffect = (CFeelEffect*) FEEL_EFFECT_DONT_CHANGE  
        );

    BOOL
	ChangeParameters( 
        LPCRECT pRectOutside,
        LONG lStiffness = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwWallWidth = FEEL_EFFECT_DONT_CHANGE,
        CFeelEffect* pInsideEffect = (CFeelEffect*) FEEL_EFFECT_DONT_CHANGE  
        );


    //
    // OPERATIONS
    //

    public:


    BOOL 
	Initialize( 
        CFeelDevice* pDevice, 
        DWORD dwWidth = FEEL_ENCLOSURE_DEFAULT_WIDTH,
        DWORD dwHeight = FEEL_ENCLOSURE_DEFAULT_HEIGHT,
        LONG lStiffness = FEEL_BOX_DEFAULT_STIFFNESS,
        DWORD dwWallWidth = FEEL_BOX_DEFAULT_WALL_WIDTH,
        POINT pntCenter = FEEL_BOX_DEFAULT_CENTER_POINT,
        CFeelEffect* pInsideEffect = NULL  
        );


    BOOL 
	Initialize( 
        CFeelDevice* pDevice, 
        LPCRECT pRectOutside,
        LONG lStiffness = FEEL_BOX_DEFAULT_STIFFNESS,
        DWORD dwWallWidth = FEEL_BOX_DEFAULT_WALL_WIDTH,
        CFeelEffect* pInsideEffect = NULL  
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


#endif // !defined(AFX_FEELBOX_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
