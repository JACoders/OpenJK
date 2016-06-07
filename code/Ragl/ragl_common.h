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
// Common
// ------
// The raven libraries contain a number of common defines, enums, and typedefs which
// need to be accessed by all templates.  Each of these is included here.
//
// Also included is a safeguarded assert file for all the asserts in RTL.
//
// This file is included in EVERY TEMPLATE, so it should be very light in order to
// reduce compile times.
//
//
// Format
// ------
// In order to simplify code and provide readability, the template library has some
// standard formats.  Any new templates or functions should adhere to these formats:
//
// - All memory is statically allocated, usually by parameter SIZE
// - All classes provide an enum which defines constant variables, including CAPACITY
// - All classes which moniter the number of items allocated provide the following functions:
//     size()   - the number of objects
//     empty()  - does the container have zero objects
//     full()   - does the container have any room left for more objects
//     clear()  - remove all objects
//
//
// - Functions are defined in the following order:
//     Capacity
//     Constructors  (copy, from string, etc...)
//     Range		 (size(), empty(), full(), clear(), etc...)
//     Access        (operator[], front(), back(), etc...)
//     Modification  (add(), remove(), push(), pop(), etc...)
//     Iteration     (begin(), end(), insert(), erase(), find(), etc...)
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RAGL_COMMON_INC)
#define RAGL_COMMON_INC







////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including ragl_common.h")
#endif
#if !defined(RAGL_ASSERT_INC)
	#define  RAGL_ASSERT_INC
	#include <assert.h>
#endif
#if !defined(FINAL_BUILD)
	#if !defined(RAGL_PROFILE_INC)
		#define  RAGL_PROFILE_INC
        #ifdef _WIN32
            #include "windows.h"
        #endif
	#endif
#endif
#if !defined(RAVL_VEC_INC)
	#include "../Ravl/CVec.h"
#endif
#if !defined(RATL_COMMON_INC)
	#include "../Ratl/ratl_common.h"
#endif
namespace ragl
{



////////////////////////////////////////////////////////////////////////////////////////
// Enums
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Typedefs
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////
// The Graph Node Class
////////////////////////////////////////////////////////////////////////////////////////
class	CNode
{
public:
	CNode()									{}
	CNode(const CVec3&	Pt) : mPoint(Pt)	{}


	////////////////////////////////////////////////////////////////////////////////////
	// Access Operator (For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	float			operator[](int dimension)
	{
		return mPoint[dimension];
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Equality Operator (For KDTree)
	////////////////////////////////////////////////////////////////////////////////////
	bool			operator==(const CNode& t) const
	{
		return (t.mPoint==mPoint);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Left Right Test (For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	virtual		ESide	LRTest(const CNode& A, const CNode& B) const
	{
		return (mPoint.LRTest(A.mPoint, B.mPoint));
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Point In Circle (For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	virtual		bool	InCircle(const CNode& A, const CNode& B, const CNode& C) const
	{
		return (mPoint.PtInCircle(A.mPoint, B.mPoint, C.mPoint));
	}



public:
	CVec3		mPoint;
};




////////////////////////////////////////////////////////////////////////////////////////
// The Graph Edge Class
////////////////////////////////////////////////////////////////////////////////////////
class	CEdge
{
public:
	int			mNodeA;
	int			mNodeB;
	bool		mOnHull;
	float		mDistance;
	bool		mCanBeInval;
	bool		mValid;
};


////////////////////////////////////////////////////////////////////////////////////////
// The Geometric Reference Class
//
// This adds one additional function to the common ratl_ref class to allow access for
// various dimensions.  It is used in both Triangulation and KDTree
////////////////////////////////////////////////////////////////////////////////////////
template <class TDATA, class TDATAREF>
class	ragl_ref
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructors
    ////////////////////////////////////////////////////////////////////////////////////
	ragl_ref()												{}
	ragl_ref(const ragl_ref & r)							{mDataRef = (TDATAREF)(r.mDataRef);}
	ragl_ref(const TDATA & r)								{mDataRef = (TDATAREF)(& r);}
	ragl_ref(const TDATAREF r)								{mDataRef = (TDATAREF)(r);}


    ////////////////////////////////////////////////////////////////////////////////////
	// Assignment Operators
    ////////////////////////////////////////////////////////////////////////////////////
	void			operator=(const ragl_ref & r)			{mDataRef = (TDATAREF)(r.mDataRef);}
	void			operator=(const TDATA & r)				{mDataRef = (TDATAREF)(& r);}
	void			operator=(const TDATAREF r)				{mDataRef = (TDATAREF)(r);}

	////////////////////////////////////////////////////////////////////////////////////
	// Access Operator (For Triangulation)
	////////////////////////////////////////////////////////////////////////////////////
	float			operator[](int dimension) const			{return (*mDataRef)[dimension];}


    ////////////////////////////////////////////////////////////////////////////////////
	// Dereference Operator
    ////////////////////////////////////////////////////////////////////////////////////
	TDATA &			operator*()								{return (*mDataRef);}
	const TDATA &	operator*() const						{return (*mDataRef);}

	TDATAREF		handle() const							{return mDataRef;}



    ////////////////////////////////////////////////////////////////////////////////////
	// Equality / Inequality Operators
    ////////////////////////////////////////////////////////////////////////////////////
	bool			operator== (const ragl_ref& t) const	{return	(*mDataRef)==(*(t.mDataRef));}
	bool			operator!= (const ragl_ref& t) const	{return	(*mDataRef)!=(*(t.mDataRef));}
	bool			operator<  (const ragl_ref& t) const	{return	(*mDataRef)< (*(t.mDataRef));}
	bool			operator>  (const ragl_ref& t) const	{return	(*mDataRef)> (*(t.mDataRef));}
	bool			operator<= (const ragl_ref& t) const	{return	(*mDataRef)<=(*(t.mDataRef));}
	bool			operator>= (const ragl_ref& t) const	{return	(*mDataRef)>=(*(t.mDataRef));}

    ////////////////////////////////////////////////////////////////////////////////////
	// Equality / Inequality Operators
    ////////////////////////////////////////////////////////////////////////////////////
	bool			operator== (const TDATA& t) const		{return	(*mDataRef)==t;}
	bool			operator!= (const TDATA& t) const		{return	(*mDataRef)!=t;}
	bool			operator<  (const TDATA& t) const		{return	(*mDataRef)< t;}
	bool			operator>  (const TDATA& t) const		{return	(*mDataRef)> t;}
	bool			operator<= (const TDATA& t) const		{return	(*mDataRef)<=t;}
	bool			operator>= (const TDATA& t) const		{return	(*mDataRef)>=t;}


    ////////////////////////////////////////////////////////////////////////////////////
	// The Data Reference
    ////////////////////////////////////////////////////////////////////////////////////
private:
	TDATAREF		mDataRef;
};

}
#endif