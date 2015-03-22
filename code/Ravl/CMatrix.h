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
// Matrix Library
// --------------
//
//
//
// NOTES:
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RAVL_MATRIX_INC)
#define RAVL_MATRIX_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if defined(RA_DEBUG_LINKING)
	#pragma message("...including CMatrix.h")
#endif
#if !defined(RAVL_VEC_INC)
	#include "CVec.h"
#endif
//namespace ravl
//{






////////////////////////////////////////////////////////////////////////////////////////
// The Matrix
////////////////////////////////////////////////////////////////////////////////////////
class CMatrix
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructors
    ////////////////////////////////////////////////////////////////////////////////////
	CMatrix()																{}
	CMatrix(const CVec4& x,const CVec4& y,const CVec4& z, const CVec4& w)	{v[0]=x;		v[1]=y;			v[2]=z;			v[3]=w;}
	CMatrix(const CMatrix& t)												{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2];	v[3]=t.v[3];}
	CMatrix(const float t[16])												{v[0]=t[0];		v[1]=t[4];		v[2]=t[8];		v[3]=t[12];}

    ////////////////////////////////////////////////////////////////////////////////////
	// Initializers
    ////////////////////////////////////////////////////////////////////////////////////
	void Set(const CVec4& x,const CVec4& y,const CVec4& z, const CVec4& w)	{v[0]=x;		v[1]=y;			v[2]=z;			v[3]=w;}
	void Set(const CMatrix& t)												{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2];	v[3]=t.v[3];}
	void Set(const float t[16])												{v[0]=t[0];		v[1]=t[4];		v[2]=t[8];		v[3]=t[12];}

	void Clear()															{v[0].Set(0,0,0,0);	v[1].Set(0,0,0,0);	v[2].Set(0,0,0,0);	v[3].Set(0,0,0,0);}
	void Itentity()															{v[0].Set(1,0,0,0);	v[1].Set(0,1,0,0);	v[2].Set(0,0,1,0);	v[3].Set(0,0,0,1);}
	void Translate(const float x, const float y, const float z)				{v[0].Set(1,0,0,0);	v[1].Set(0,1,0,0);	v[2].Set(0,0,1,0);	v[3].Set(x,y,z,1);}
	void Scale(const float x, const float y, const float z)					{v[0].Set(x,0,0,0);	v[1].Set(0,y,0,0);	v[2].Set(0,0,z,0);	v[3].Set(0,0,0,1);}
	void Rotate(int axis, const float s/*sin(angle)*/, const float c/*cos(angle)*/)
	{
		switch(axis)
		{
		case 0:
			v[0].Set( 1, 0, 0, 0);
			v[1].Set( 0, c,-s, 0);
			v[2].Set( 0, s, c, 0);
			break;
		case 1:
			v[0].Set( c, 0, s, 0);
			v[1].Set( 0, 1, 0, 0);
			v[2].Set(-s, 0, c, 0);
			break;
		case 2:
			v[0].Set( c,-s, 0, 0);
			v[1].Set( s, c, 0, 0);
			v[2].Set( 0, 0, 1, 0);
			break;
		}
		v[3].Set( 0, 0, 0, 1);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Member Accessors
    ////////////////////////////////////////////////////////////////////////////////////
	const CVec4& operator[](int i) const 									{return v[i];}
	CVec4& operator[](int i)												{return v[i];}

	CVec4& up()																{return v[0];}
	CVec4& left()															{return v[1];}
	CVec4& fwd()															{return v[2];}
	CVec4& origin()															{return v[3];}

    ////////////////////////////////////////////////////////////////////////////////////
	// Equality / Inequality Operators
    ////////////////////////////////////////////////////////////////////////////////////
	bool operator== (const CMatrix& t) const								{return	 (v[0]==t.v[0] && v[1]==t.v[1] && v[2]==t.v[2] && v[3]==t.v[3]);}
	bool operator!= (const CMatrix& t) const								{return !(v[0]==t.v[0] && v[1]==t.v[1] && v[2]==t.v[2] && v[3]==t.v[3]);}

    ////////////////////////////////////////////////////////////////////////////////////
	// Basic Arithimitic Operators
    ////////////////////////////////////////////////////////////////////////////////////
	const CMatrix &operator=  (const CMatrix& t)							{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2];	v[3]=t.v[3]; return *this;}
	const CMatrix &operator+= (const CMatrix& t)							{v[0]+=t.v[0];	v[1]+=t.v[1];	v[2]+=t.v[2];	v[3]+=t.v[3];return *this;}
	const CMatrix &operator-= (const CMatrix& t)							{v[0]-=t.v[0];	v[1]-=t.v[1];	v[2]-=t.v[2];	v[3]-=t.v[3];return *this;}

	CMatrix		   operator+ (const CMatrix &t) const						{return CMatrix(v[0]+t.v[0], v[1]+t.v[1], v[2]+t.v[2], v[3]+t.v[3]);}
	CMatrix		   operator- (const CMatrix &t) const						{return CMatrix(v[0]-t.v[0], v[1]-t.v[1], v[2]-t.v[2], v[3]-t.v[3]);}

    ////////////////////////////////////////////////////////////////////////////////////
	// Matrix Scale
    ////////////////////////////////////////////////////////////////////////////////////
	const CMatrix &operator*= (const float d)								{v[0]*=d;		v[1]*=d;		v[2]*=d;		v[3]*=d;	 return *this;}

	
    ////////////////////////////////////////////////////////////////////////////////////
	// Matrix To Matrix Multiply
    ////////////////////////////////////////////////////////////////////////////////////
	CMatrix		   operator* (const CMatrix &t) const
	{
	//	assert(this!=&t);				// Don't Multiply With Self

		CMatrix		Result;				// The Resulting Matrix
		int			i,j,k;				// Counters
		float		Accumulator;		// Current Value Of The Dot Product
		for (i=0; i<4; i++)
		{
			for (j=0; j<4; j++)
			{
				Accumulator = 0.0f;		// Reset The Accumulator
				for(k=0; k<4; k++)
				{
					Accumulator += v[i][k]*t[k][j];		// Calculate Dot Product Of The Two Vectors
				}
				Result[i][j]=Accumulator;	// Place In Result
			}
		}

		return Result;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Vector To Matrix Multiply
    ////////////////////////////////////////////////////////////////////////////////////
	CVec4		   operator* (const CVec4 &t)   const
	{
		CVec4		Result;

		Result[0] = v[0][0]*t[0] + v[1][0]*t[1] + v[2][0]*t[2] + v[3][0];
		Result[1] = v[0][1]*t[0] + v[1][1]*t[1] + v[2][1]*t[2] + v[3][1];
		Result[2] = v[0][2]*t[0] + v[1][2]*t[1] + v[2][2]*t[2] + v[3][2];

		return Result;
	}

public:
	CVec4		v[4];
};



//}
#endif
