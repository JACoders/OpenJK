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

  FILE:		FeelGrid.h

  PURPOSE:	Feelit API Grid Effect Class

  STARTED:	12/11/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELGrid_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELGrid_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include <windows.h>
#include "FeelCondition.h"


//================================================================
// Constants
//================================================================

#define     FEEL_GRID_DEFAULT_HORIZ_OFFSET     0  
#define     FEEL_GRID_DEFAULT_VERT_OFFSET      0
#define     FEEL_GRID_DEFAULT_HORIZ_SPACING    100
#define     FEEL_GRID_DEFAULT_VERT_SPACING     100
#define     FEEL_GRID_DEFAULT_NODE_STRENGTH    5000
#define     FEEL_GRID_DEFAULT_NODE_SATURATION  10000   

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_GRID_DEFAULT_HORIZ_OFFSET     FEEL_GRID_DEFAULT_HORIZ_OFFSET 
#define     FORCE_GRID_DEFAULT_VERT_OFFSET      FEEL_GRID_DEFAULT_VERT_OFFSET
#define     FORCE_GRID_DEFAULT_HORIZ_SPACING    FEEL_GRID_DEFAULT_HORIZ_SPACING
#define     FORCE_GRID_DEFAULT_VERT_SPACING     FEEL_GRID_DEFAULT_VERT_SPACING
#define     FORCE_GRID_DEFAULT_NODE_STRENGTH    FEEL_GRID_DEFAULT_NODE_STRENGTH
#define     FORCE_GRID_DEFAULT_NODE_SATURATION  FEEL_GRID_DEFAULT_NODE_SATURATION


//================================================================
// CFeelGrid
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelGrid : public CFeelCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CFeelGrid();

	// Destructor
	virtual
	~CFeelGrid();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

    BOOL
    ChangeParameters( 
        DWORD dwHorizSpacing,
        DWORD dwVertSpacing,
        LONG  lHorizNodeStrength = FEEL_EFFECT_DONT_CHANGE,
        LONG  lVertNodeStrength = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwHorizOffset = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwVertOffset = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwHorizNodeSaturation = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwVertNodeSaturation = FEEL_EFFECT_DONT_CHANGE
        );


    //
    // OPERATIONS
    //

    public:

    virtual BOOL 
    Initialize( 
        CFeelDevice* pDevice, 
        DWORD dwHorizSpacing = FEEL_GRID_DEFAULT_HORIZ_SPACING,
        DWORD dwVertSpacing = FEEL_GRID_DEFAULT_VERT_SPACING,
        LONG  lHorizNodeStrength = FEEL_GRID_DEFAULT_NODE_STRENGTH,
        LONG  lVertNodeStrength = FEEL_GRID_DEFAULT_NODE_STRENGTH,
        DWORD dwHorizOffset = FEEL_GRID_DEFAULT_HORIZ_OFFSET,
        DWORD dwVertOffset = FEEL_GRID_DEFAULT_VERT_OFFSET,
        DWORD dwHorizNodeSaturation = FEEL_GRID_DEFAULT_NODE_SATURATION,
        DWORD dwVertNodeSaturation = FEEL_GRID_DEFAULT_NODE_SATURATION
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



//
// INLINES
//

inline BOOL
CFeelGrid::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Feel_Grid);
}
		
#endif // !defined(AFX_FEELGrid_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
