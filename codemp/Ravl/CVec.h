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

#pragma once

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD TEMPLATE LIBRARY
//  (c) 2002 Activision
//
//
// Vector Library
// --------------
// The base implimention of the Raven Vector object attempts to solve a number of
// high level problems as efficiently as possible.  Where ever feasible, functions have
// been included in the .h file so the compiler can inline them.
//
// The vectors define the following operations:
//  - Construction
//  - Initialization
//  - Member Access
//  - Equality / Inequality Operators
//  - Arithimitic Operators
//  - Length & Distance
//  - Normalization (Standard, Safe, Angular)
//  - Dot & Cross Product
//  - Perpendicular Vector
//  - Truncation
//  - Min & Max Element Analisis
//  - Interpolation
//  - Angle / Vector Conversion
//  - Translation & Rotation
//  - Point and Line Intersection Tests
//  - Left / Right Line Test
//  - String Operations
//  - Debug Routines
//  - "Standard" Vectors As Static Memebers
//
// As necessary, some projects may #define special faster versions of these routines to
// make better use of native hardware / software implimentations.
//
//
//
//
// NOTES:
// 05/29/02 - CREATED
// 05/30/02 - RotatePoint() is currently unimplimented.  Waiting for Matrix Library
//
//
////////////////////////////////////////////////////////////////////////////////////////

//namespace ravl
//{


template <class T> T	Min(const T& a, const T& b)	{return (a<b)?(a):(b);}
template <class T> T	Max(const T& a, const T& b)	{return (b<a)?(a):(b);}



////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define		RAVL_VEC_UDF					(1.234567E-10f)						// Undefined Vector Value (for debugging)
#define		RAVL_VEC_PI						(3.1415926535f)						// Pi
#define		RAVL_VEC_DEGTORADCONST			(0.0174532925f)						// (RAVL_VEC_PI / 180.0f)
#define		RAVL_VEC_RADTODEGCONST			(57.295779514f)						// (180.0f / RAVL_VEC_PI)
#define		RAVL_VEC_DEGTORAD( a )			( (a) * RAVL_VEC_DEGTORADCONST )	// Quick Macro For Degrees -> Radians
#define		RAVL_VEC_RADTODEG( a )			( (a) * RAVL_VEC_RADTODEGCONST )	// Quick Macro For Radians -> Degrees



////////////////////////////////////////////////////////////////////////////////////////
// Enums And Typedefs
////////////////////////////////////////////////////////////////////////////////////////
enum	ESide
{
	Side_None	= 0,
	Side_Left	= 1,
	Side_Right	= 2,
	Side_In		= 3,
	Side_Out	= 4,
	Side_AllIn	= 5
};





////////////////////////////////////////////////////////////////////////////////////////
// The 4 Dimensional Vector
////////////////////////////////////////////////////////////////////////////////////////
class CVec4
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructors
    ////////////////////////////////////////////////////////////////////////////////////
#ifndef _DEBUG
	CVec4()																	{}
#else
	CVec4()																	{v[0]=v[1]=v[2]=v[3]=RAVL_VEC_UDF;}		// DEBUG INITIALIZATION
#endif
	CVec4(const float val)													{v[0]=val;		v[1]=val;		v[2]=val;		v[3]=val;}
	CVec4(const float x,const float y,const float z, const float r)			{v[0]=x;		v[1]=y;			v[2]=z;			v[3]=r;}
	CVec4(const CVec4& t)													{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2];	v[3]=t.v[3];}
	CVec4(const float *t)													{v[0]=t[0];		v[1]=t[1];		v[2]=t[2];		v[3]=t[3];}

    ////////////////////////////////////////////////////////////////////////////////////
	// Initializers
    ////////////////////////////////////////////////////////////////////////////////////
	void Set(const float t)													{v[0]=t;		v[1]=t;			v[2]=t;			v[3]=t;}
	void Set(const float *t)												{v[0]=t[0];		v[1]=t[1];		v[2]=t[2];		v[3]=t[3];}
	void Set(const float x,const float y,const float z, const float r)		{v[0]=x;		v[1]=y;			v[2]=z;			v[3]=r;}
	void Clear()															{v[0]=0;		v[1]=0;			v[2]=0;			v[3]=0;}

    ////////////////////////////////////////////////////////////////////////////////////
	// Member Accessors
    ////////////////////////////////////////////////////////////////////////////////////
	const float& operator[](int i) const 									{return v[i];}
	float& operator[](int i)												{return v[i];}
	float& pitch()															{return v[0];}
	float& yaw()															{return v[1];}
	float& roll()															{return v[2];}
	float& radius()															{return v[3];}

    ////////////////////////////////////////////////////////////////////////////////////
	// Equality / Inequality Operators
    ////////////////////////////////////////////////////////////////////////////////////
	bool operator!  () const												{return	!(v[0]         && v[1]         && v[2]         && v[3]        );}
	bool operator== (const CVec4& t) const									{return	 (v[0]==t.v[0] && v[1]==t.v[1] && v[2]==t.v[2] && v[3]==t.v[3]);}
	bool operator!= (const CVec4& t) const									{return !(v[0]==t.v[0] && v[1]==t.v[1] && v[2]==t.v[2] && v[3]==t.v[3]);}
	bool operator<  (const CVec4& t) const									{return	 (v[0]< t.v[0] && v[1]< t.v[1] && v[2]< t.v[2] && v[3]< t.v[3]);}
	bool operator>  (const CVec4& t) const									{return	 (v[0]> t.v[0] && v[1]> t.v[1] && v[2]> t.v[2] && v[3]> t.v[3]);}
	bool operator<= (const CVec4& t) const									{return	 (v[0]<=t.v[0] && v[1]<=t.v[1] && v[2]<=t.v[2] && v[3]<=t.v[3]);}
	bool operator>= (const CVec4& t) const									{return	 (v[0]>=t.v[0] && v[1]>=t.v[1] && v[2]>=t.v[2] && v[3]>=t.v[3]);}

    ////////////////////////////////////////////////////////////////////////////////////
	// Basic Arithimitic Operators
    ////////////////////////////////////////////////////////////////////////////////////
	const CVec4 &operator=  (const float d)									{v[0]=d;		v[1]=d;			v[2]=d;			v[3]=d;		 return *this;}
	const CVec4 &operator=  (const CVec4& t)								{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2];	v[3]=t.v[3]; return *this;}

	const CVec4 &operator+= (const float d)									{v[0]+=d;		v[1]+=d;		v[2]+=d;		v[3]+=d;	 return *this;}
	const CVec4 &operator+= (const CVec4& t)								{v[0]+=t.v[0];	v[1]+=t.v[1];	v[2]+=t.v[2];	v[3]+=t.v[3];return *this;}

	const CVec4 &operator-= (const float d)									{v[0]-=d;		v[1]-=d;		v[2]-=d; 		v[3]-=d;	 return *this;}
	const CVec4 &operator-= (const CVec4& t)								{v[0]-=t.v[0];	v[1]-=t.v[1];	v[2]-=t.v[2];	v[3]-=t.v[3];return *this;}

	const CVec4 &operator*= (const float d)									{v[0]*=d;		v[1]*=d;		v[2]*=d;		v[3]*=d;	 return *this;}
	const CVec4 &operator*= (const CVec4& t)								{v[0]*=t.v[0];	v[1]*=t.v[1];	v[2]*=t.v[2];	v[3]*=t.v[3];return *this;}

	const CVec4 &operator/= (const float d)									{v[0]/=d;		v[1]/=d;		v[2]/=d;		v[3]/=d;	 return *this;}
	const CVec4 &operator/= (const CVec4& t)								{v[0]/=t.v[0];	v[1]/=t.v[1];	v[2]/=t.v[2];	v[3]/=t.v[3];return *this;}

	inline CVec4 operator+ (const CVec4 &t) const							{return CVec4(v[0]+t.v[0], v[1]+t.v[1], v[2]+t.v[2], v[3]+t.v[3]);}
	inline CVec4 operator- (const CVec4 &t) const							{return CVec4(v[0]-t.v[0], v[1]-t.v[1], v[2]-t.v[2], v[3]-t.v[3]);}
	inline CVec4 operator* (const CVec4 &t) const							{return CVec4(v[0]*t.v[0], v[1]*t.v[1], v[2]*t.v[2], v[3]*t.v[3]);}
	inline CVec4 operator/ (const CVec4 &t) const							{return CVec4(v[0]/t.v[0], v[1]/t.v[1], v[2]/t.v[2], v[3]/t.v[3]);}


    ////////////////////////////////////////////////////////////////////////////////////
	// Length And Distance Calculations
    ////////////////////////////////////////////////////////////////////////////////////
	float	Len() const;
	float	Len2() const													{return (v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+v[3]*v[3]);}

	float	Dist(const CVec4& t) const;
	float	Dist2(const CVec4& t) const										{return ((t.v[0]-v[0])*(t.v[0]-v[0]) + (t.v[1]-v[1])*(t.v[1]-v[1]) + (t.v[2]-v[2])*(t.v[2]-v[2]) + (t.v[3]-v[3])*(t.v[3]-v[3]) );}


    ////////////////////////////////////////////////////////////////////////////////////
	// Normalization
    ////////////////////////////////////////////////////////////////////////////////////
	float	Norm();
	float	SafeNorm();
	void	AngleNorm();


    ////////////////////////////////////////////////////////////////////////////////////
	// Dot, Cross & Perpendicular Vector
    ////////////////////////////////////////////////////////////////////////////////////
	float	Dot(const CVec4& t) const										{return (v[0]*t.v[0] + v[1]*t.v[1] + v[2]*t.v[2] + v[3]*t.v[3]);}
	void	Cross(const CVec4& t)
	{
		CVec4 temp(*this);
		v[0] = (temp.v[1]*t.v[2]) - (temp.v[2]*t.v[1]);
		v[1] = (temp.v[2]*t.v[0]) - (temp.v[0]*t.v[2]);
		v[2] = (temp.v[0]*t.v[1]) - (temp.v[1]*t.v[0]);
		v[3] = 0;
	}
	void	Perp();


    ////////////////////////////////////////////////////////////////////////////////////
	// Truncation & Element Analysis
    ////////////////////////////////////////////////////////////////////////////////////
	void	Min(const CVec4& t)
	{
		if (t.v[0]<v[0])			v[0]=t.v[0];
		if (t.v[1]<v[1])			v[1]=t.v[1];
		if (t.v[2]<v[2])			v[2]=t.v[2];
		if (t.v[3]<v[3])			v[3]=t.v[3];
	}
	void	Max(const CVec4& t)
	{
		if (t.v[0]>v[0])			v[0]=t.v[0];
		if (t.v[1]>v[1])			v[1]=t.v[1];
		if (t.v[2]>v[2])			v[2]=t.v[2];
		if (t.v[3]>v[3])			v[3]=t.v[3];
	}
	float	MaxElement() const
	{
			return v[MaxElementIndex()];
	}
	int		MaxElementIndex() const;


    ////////////////////////////////////////////////////////////////////////////////////
	// Interpolation
    ////////////////////////////////////////////////////////////////////////////////////
	void	Interp(const CVec4 &v1, const CVec4 &v2, const float t)
	{
		(*this)=v1;
		(*this)-=v2;
		(*this)*=t;
		(*this)+=v2;
	}
	void	ScaleAdd(const CVec4& t, const float scale)
	{
		v[0] += (scale * t.v[0]);
		v[1] += (scale * t.v[1]);
		v[2] += (scale * t.v[2]);
		v[3] += (scale * t.v[3]);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Conversion Angle To Vector (Angle In Degrees)
    ////////////////////////////////////////////////////////////////////////////////////
	void	VecToAng();
	void	AngToVec();
	void	AngToVec(CVec4& Right, CVec4& Up);

    ////////////////////////////////////////////////////////////////////////////////////
	// Conversion Angle To Vector (Angle In Radians)
    ////////////////////////////////////////////////////////////////////////////////////
	void	VecToAngRad();
	void	AngToVecRad();
	void	AngToVecRad(CVec4& Right, CVec4& Up);

    ////////////////////////////////////////////////////////////////////////////////////
	// Conversion Between Radians And Degrees
    ////////////////////////////////////////////////////////////////////////////////////
	void	ToRadians();
	void	ToDegrees();



    ////////////////////////////////////////////////////////////////////////////////////
	// Project
	//
	// Standard projection function.  Take the (this) and project it onto the vector
	// (U).  Imagine drawing a line perpendicular to U from the endpoint of the (this)
	// Vector.  That then becomes the new vector.
	//
	// The value returned is the scale of the new vector with respect to the one passed
	// to the function.  If the scale is less than (1.0) then the new vector is shorter
	// than (U).  If the scale is negative, then the vector is going in the opposite
	// direction of (U).
	//
	//               _  (U)
	//               /|
	//             /                                        _ (this)
	//           /                      RESULTS->           /|
	//         /                                          /
	//       /    __\ (this)                            /
	//     /___---  /                                 /
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	Project(const CVec4 &U)
	{
		float	Scale = (Dot(U) / U.Len2());	// Find the scale of this vector on U
		(*this)=U;								// Copy U onto this vector
		(*this)*=Scale;							// Use the previously calculated scale to get the right length.
		return Scale;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Project To Line
	//
	// This function takes two other points in space as the start and end of a line
	// segment and projects the (this) point onto the line defined by (Start)->(Stop)
	//
	// RETURN VALUES:
	//   (-INF, 0.0)  : (this) landed on the line before (Start)
	//   (0.0, 1.0)   : (this) landed in the line segment between (Start) and (Stop)
	//   (1.0, INF)   : (this) landed on the line beyond (End)
	//
	//             (Stop)
	//               /
	//             /
	//           o _
	//         /  |\
	//       /     (this)
	//     /
	// (Start)
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	ProjectToLine(const CVec4 &Start, const CVec4 &Stop)
	{
		(*this) -= Start;
		float	Scale = Project(Stop - Start);
		(*this) += Start;
		return Scale;
	}

    ////////////////////////////////////////////////////////////////////////////////////
    // Project To Line Seg
	//
	// Same As Project To Line, Except It Will Clamp To Start And Stop
	////////////////////////////////////////////////////////////////////////////////////
	float	ProjectToLineSeg(const CVec4 &Start, const CVec4 &Stop)
	{
		float Scale = ProjectToLine(Start, Stop);
		if (Scale<0.0f)
		{
			(*this) = Start;
		}
		else if (Scale>1.0f)
		{
			(*this) = Stop;
		}
		return Scale;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Distance To Line
	//
	// Uses project to line and than calculates distance to the new point
	////////////////////////////////////////////////////////////////////////////////////
	float	DistToLine(const CVec4 &Start, const CVec4 &Stop) const
	{
		CVec4	P(*this);
		P.ProjectToLineSeg(Start, Stop);

		return Dist(P);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Distance To Line
	//
	// Uses project to line and than calculates distance to the new point
	////////////////////////////////////////////////////////////////////////////////////
	float	DistToLine2(const CVec4 &Start, const CVec4 &Stop) const
	{
		CVec4	P(*this);
		P.ProjectToLineSeg(Start, Stop);

		return Dist2(P);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Translation & Rotation (2D)
    ////////////////////////////////////////////////////////////////////////////////////
	void	RotatePoint(const CVec4 &Angle, const CVec4 &Origin);
	void	Reposition(const CVec4 &Translation, float RotationDegrees=0.0);



    ////////////////////////////////////////////////////////////////////////////////////
	// Area Of The Parallel Pipid (2D)
	//
	// Given two more points, this function calculates the area of the parallel pipid
	// formed.
	//
	// Note: This function CAN return a negative "area" if (this) is above or right of
	// (A) and (B)...  We do not take the abs because the sign of the "area" is needed
	// for the left right test (see below)
	//
	//
	//               ___---( ... )
	//        (A)---/        /
	//        /             /
	//       /             /
	//      /             /
	//     /      ___---(B)
	//  (this)---/
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	AreaParallelPipid(const CVec4 &A, const CVec4 &B) const
	{
		return ((A.v[0]*B.v[1] - A.v[1]*B.v[0]) +
			    (B.v[0]*  v[1] -   v[0]*B.v[1]) +
				(  v[0]*A.v[1] - A.v[0]*  v[1]));
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Area Of The Triangle (2D)
	//
	// Given two more points, this function calculates the area of the triangle formed.
	//
	//        (A)
	//        /  \__
	//       /      \__
	//      /          \_
	//     /      ___---(B)
	//  (this)---/
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	AreaTriange(const CVec4 &A, const CVec4 &B) const
	{
		return (AreaParallelPipid(A, B) * 0.5f);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// The Left Right Test (2D)
	//
	// Given a line segment (Start->End) and a tolerance for *right on*, this function
	// evaluates which side the point is of the line.  (Side_Left in this example)
	//
	//
	//
	//          (this)        ___---/(End)
	//                 ___---/
	//          ___---/
	//  (Start)/
	//
    ////////////////////////////////////////////////////////////////////////////////////
	ESide	LRTest(const CVec4 &Start, const CVec4 &End, float Tolerance=0.0) const
	{
		float Area = AreaParallelPipid(Start, End);
		if (Area>Tolerance)
		{
			return Side_Left;
		}
		if (Area<(Tolerance*-1))
		{
			return Side_Right;
		}
		return Side_None;

	}

	////////////////////////////////////////////////////////////////////////////////////
	// Point In Circumscribed Circle  (True/False)
	//
	//  Returns true if the given point is within the circumscribed
	//  circle of the given ABC Triangle:
	//         _____
	//        /   B \
	//      /   /   \ \
	//     |  /      \ |
	//     |A---------C|
	//      \    Pt   /
	//       \_______/
	//
	////////////////////////////////////////////////////////////////////////////////////
	bool	PtInCircle(const CVec4 &A, const CVec4 &B, const CVec4 &C) const;


	////////////////////////////////////////////////////////////////////////////////////
	// Point In Standard Circle  (True/False)
	//
	//  Returns true if the given point is within the Circle
	//         _____
	//        /     \
	//      /         \
	//     |   Circle  |
	//     |           |
	//      \    Pt   /
	//       \_______/
	//
	////////////////////////////////////////////////////////////////////////////////////
	bool	PtInCircle(const CVec4 &Circle, float Radius) const;

	////////////////////////////////////////////////////////////////////////////////////
	// Line Intersects Circle  (True/False)
	//
	//  r	- Radius Of The Circle
	//  A	- Start Of Line Segment
	//  B	- End Of Line Segment
	//
	//  P	- Projected Position Of Origin Onto Line AB
	//
	//
	//            (Stop)
	//              /
	//            /
	//         (P)
	//        /   \      \
	//      /   (this)-r->|
	//    /              /
	// (Start)
	//
	////////////////////////////////////////////////////////////////////////////////////
	bool	LineInCircle(const CVec4 &Start, const CVec4 &Stop, float Radius);
	bool	LineInCircle(const CVec4 &Start, const CVec4 &Stop, float Radius, CVec4 &PointOnLine);



    ////////////////////////////////////////////////////////////////////////////////////
	// String Operations
    ////////////////////////////////////////////////////////////////////////////////////
	void	FromStr(const char *s);
	void	ToStr(char* s) const;


    ////////////////////////////////////////////////////////////////////////////////////
	// Debug Routines
    ////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	bool	IsFinite();
	bool	IsInitialized();
#endif


    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////
private:
	float v[4];


public:
	static const CVec4 mX;
	static const CVec4 mY;
	static const CVec4 mZ;
	static const CVec4 mW;
	static const CVec4 mZero;
};



























////////////////////////////////////////////////////////////////////////////////////////
// The 3 Dimensional Vector
////////////////////////////////////////////////////////////////////////////////////////
class CVec3
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructors
    ////////////////////////////////////////////////////////////////////////////////////
#ifndef _DEBUG
	CVec3()																	{}
#else
	CVec3()																	{v[0]=v[1]=v[2]=RAVL_VEC_UDF;}		// DEBUG INITIALIZATION
#endif
	CVec3(const float val)													{v[0]=val;		v[1]=val;		v[2]=val;	}
	CVec3(const float x,const float y,const float z)						{v[0]=x;		v[1]=y;			v[2]=z;		}
	CVec3(const CVec3& t)													{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2];}
	CVec3(const float *t)													{v[0]=t[0];		v[1]=t[1];		v[2]=t[2];	}

	float	x()		const													{return v[0];}
	float	y()		const													{return v[1];}
	float	z()		const													{return v[2];}

    ////////////////////////////////////////////////////////////////////////////////////
	// Initializers
    ////////////////////////////////////////////////////////////////////////////////////
	void Set(const float t)													{v[0]=t;		v[1]=t;			v[2]=t;		}
	void Set(const float *t)												{v[0]=t[0];		v[1]=t[1];		v[2]=t[2];	}
	void Set(const float x,const float y,const float z)						{v[0]=x;		v[1]=y;			v[2]=z;		}
	void Clear()															{v[0]=0;		v[1]=0;			v[2]=0;		}

    ////////////////////////////////////////////////////////////////////////////////////
	// Member Accessors
    ////////////////////////////////////////////////////////////////////////////////////
	const float& operator[](int i) const 									{return v[i];}
	float& operator[](int i)												{return v[i];}
	float& pitch()															{return v[0];}
	float& yaw()															{return v[1];}
	float& roll()															{return v[2];}

    ////////////////////////////////////////////////////////////////////////////////////
	// Equality / Inequality Operators
    ////////////////////////////////////////////////////////////////////////////////////
	bool operator!  () const												{return	!(v[0]         && v[1]         && v[2]        );}
	bool operator== (const CVec3& t) const									{return	 (v[0]==t.v[0] && v[1]==t.v[1] && v[2]==t.v[2]);}
	bool operator!= (const CVec3& t) const									{return !(v[0]==t.v[0] && v[1]==t.v[1] && v[2]==t.v[2]);}
	bool operator<  (const CVec3& t) const									{return	 (v[0]< t.v[0] && v[1]< t.v[1] && v[2]< t.v[2]);}
	bool operator>  (const CVec3& t) const									{return	 (v[0]> t.v[0] && v[1]> t.v[1] && v[2]> t.v[2]);}
	bool operator<= (const CVec3& t) const									{return	 (v[0]<=t.v[0] && v[1]<=t.v[1] && v[2]<=t.v[2]);}
	bool operator>= (const CVec3& t) const									{return	 (v[0]>=t.v[0] && v[1]>=t.v[1] && v[2]>=t.v[2]);}

    ////////////////////////////////////////////////////////////////////////////////////
	// Basic Arithimitic Operators
    ////////////////////////////////////////////////////////////////////////////////////
	const CVec3 &operator=  (const float d)									{v[0]=d;		v[1]=d;			v[2]=d;		 return *this;}
	const CVec3 &operator=  (const CVec3& t)								{v[0]=t.v[0];	v[1]=t.v[1];	v[2]=t.v[2]; return *this;}

	const CVec3 &operator+= (const float d)									{v[0]+=d;		v[1]+=d;		v[2]+=d;	 return *this;}
	const CVec3 &operator+= (const CVec3& t)								{v[0]+=t.v[0];	v[1]+=t.v[1];	v[2]+=t.v[2];return *this;}

	const CVec3 &operator-= (const float d)									{v[0]-=d;		v[1]-=d;		v[2]-=d; 	 return *this;}
	const CVec3 &operator-= (const CVec3& t)								{v[0]-=t.v[0];	v[1]-=t.v[1];	v[2]-=t.v[2];return *this;}

	const CVec3 &operator*= (const float d)									{v[0]*=d;		v[1]*=d;		v[2]*=d;	 return *this;}
	const CVec3 &operator*= (const CVec3& t)								{v[0]*=t.v[0];	v[1]*=t.v[1];	v[2]*=t.v[2];return *this;}

	const CVec3 &operator/= (const float d)									{v[0]/=d;		v[1]/=d;		v[2]/=d;	 return *this;}
	const CVec3 &operator/= (const CVec3& t)								{v[0]/=t.v[0];	v[1]/=t.v[1];	v[2]/=t.v[2];return *this;}

	inline CVec3 operator+ (const CVec3 &t) const							{return CVec3(v[0]+t.v[0], v[1]+t.v[1], v[2]+t.v[2]);}
	inline CVec3 operator- (const CVec3 &t) const							{return CVec3(v[0]-t.v[0], v[1]-t.v[1], v[2]-t.v[2]);}
	inline CVec3 operator* (const CVec3 &t) const							{return CVec3(v[0]*t.v[0], v[1]*t.v[1], v[2]*t.v[2]);}
	inline CVec3 operator/ (const CVec3 &t) const							{return CVec3(v[0]/t.v[0], v[1]/t.v[1], v[2]/t.v[2]);}


    ////////////////////////////////////////////////////////////////////////////////////
	// Length And Distance Calculations
    ////////////////////////////////////////////////////////////////////////////////////
	float	Len() const;
	float	Len2() const													{return (v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);}

	float	Dist(const CVec3& t) const;
	float	Dist2(const CVec3& t) const										{return ((t.v[0]-v[0])*(t.v[0]-v[0]) + (t.v[1]-v[1])*(t.v[1]-v[1]) + (t.v[2]-v[2])*(t.v[2]-v[2]));}


    ////////////////////////////////////////////////////////////////////////////////////
	// Normalization
    ////////////////////////////////////////////////////////////////////////////////////
	float	Norm();
	float	SafeNorm();
	void	AngleNorm();
	float	Truncate(float maxlen);


    ////////////////////////////////////////////////////////////////////////////////////
	// Dot, Cross & Perpendicular Vector
    ////////////////////////////////////////////////////////////////////////////////////
	float	Dot(const CVec3& t) const										{return (v[0]*t.v[0] + v[1]*t.v[1] + v[2]*t.v[2]);}
	void	Cross(const CVec3& t)
	{
		CVec3 temp(*this);
		v[0] = (temp.v[1]*t.v[2]) - (temp.v[2]*t.v[1]);
		v[1] = (temp.v[2]*t.v[0]) - (temp.v[0]*t.v[2]);
		v[2] = (temp.v[0]*t.v[1]) - (temp.v[1]*t.v[0]);
	}
	void	Perp();


    ////////////////////////////////////////////////////////////////////////////////////
	// Truncation & Element Analysis
    ////////////////////////////////////////////////////////////////////////////////////
	void	Min(const CVec3& t)
	{
		if (t.v[0]<v[0])			v[0]=t.v[0];
		if (t.v[1]<v[1])			v[1]=t.v[1];
		if (t.v[2]<v[2])			v[2]=t.v[2];
	}
	void	Max(const CVec3& t)
	{
		if (t.v[0]>v[0])			v[0]=t.v[0];
		if (t.v[1]>v[1])			v[1]=t.v[1];
		if (t.v[2]>v[2])			v[2]=t.v[2];
	}
	float	MaxElement() const
	{
			return v[MaxElementIndex()];
	}
	int		MaxElementIndex() const;


    ////////////////////////////////////////////////////////////////////////////////////
	// Interpolation
    ////////////////////////////////////////////////////////////////////////////////////
	void	Interp(const CVec3 &v1, const CVec3 &v2, const float t)
	{
		(*this)=v1;
		(*this)-=v2;
		(*this)*=t;
		(*this)+=v2;
	}
	void	ScaleAdd(const CVec3& t, const float scale)
	{
		v[0] += (scale * t.v[0]);
		v[1] += (scale * t.v[1]);
		v[2] += (scale * t.v[2]);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Conversion Angle To Vector (Angle In Degrees)
    ////////////////////////////////////////////////////////////////////////////////////
	void	VecToAng();
	void	AngToVec();
	void	AngToVec(CVec3& Right, CVec3& Up);

    ////////////////////////////////////////////////////////////////////////////////////
	// Conversion Angle To Vector (Angle In Radians)
    ////////////////////////////////////////////////////////////////////////////////////
	void	VecToAngRad();
	void	AngToVecRad();
	void	AngToVecRad(CVec3& Right, CVec3& Up);

    ////////////////////////////////////////////////////////////////////////////////////
	// Conversion Between Radians And Degrees
    ////////////////////////////////////////////////////////////////////////////////////
	void	ToRadians();
	void	ToDegrees();



    ////////////////////////////////////////////////////////////////////////////////////
	// Project
	//
	// Standard projection function.  Take the (this) and project it onto the vector
	// (U).  Imagine drawing a line perpendicular to U from the endpoint of the (this)
	// Vector.  That then becomes the new vector.
	//
	// The value returned is the scale of the new vector with respect to the one passed
	// to the function.  If the scale is less than (1.0) then the new vector is shorter
	// than (U).  If the scale is negative, then the vector is going in the opposite
	// direction of (U).
	//
	//               _  (U)
	//               /|
	//             /                                        _ (this)
	//           /                      RESULTS->           /|
	//         /                                          /
	//       /    __\ (this)                            /
	//     /___---  /                                 /
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	Project(const CVec3 &U)
	{
		float	Scale = (Dot(U) / U.Len2());	// Find the scale of this vector on U
		(*this)=U;								// Copy U onto this vector
		(*this)*=Scale;							// Use the previously calculated scale to get the right length.
		return Scale;
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Project To Line
	//
	// This function takes two other points in space as the start and end of a line
	// segment and projects the (this) point onto the line defined by (Start)->(Stop)
	//
	// RETURN VALUES:
	//   (-INF, 0.0)  : (this) landed on the line before (Start)
	//   (0.0, 1.0)   : (this) landed in the line segment between (Start) and (Stop)
	//   (1.0, INF)   : (this) landed on the line beyond (End)
	//
	//             (Stop)
	//               /
	//             /
	//           o _
	//         /  |\
	//       /     (this)
	//     /
	// (Start)
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	ProjectToLine(const CVec3 &Start, const CVec3 &Stop)
	{
		(*this) -= Start;
		float	Scale = Project(Stop - Start);
		(*this) += Start;
		return Scale;
	}

    ////////////////////////////////////////////////////////////////////////////////////
    // Project To Line Seg
	//
	// Same As Project To Line, Except It Will Clamp To Start And Stop
	////////////////////////////////////////////////////////////////////////////////////
	float	ProjectToLineSeg(const CVec3 &Start, const CVec3 &Stop)
	{
		float Scale = ProjectToLine(Start, Stop);
		if (Scale<0.0f)
		{
			(*this) = Start;
		}
		else if (Scale>1.0f)
		{
			(*this) = Stop;
		}
		return Scale;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Distance To Line
	//
	// Uses project to line and than calculates distance to the new point
	////////////////////////////////////////////////////////////////////////////////////
	float	DistToLine(const CVec3 &Start, const CVec3 &Stop) const
	{
		CVec3	P(*this);
		P.ProjectToLineSeg(Start, Stop);

		return Dist(P);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Distance To Line
	//
	// Uses project to line and than calculates distance to the new point
	////////////////////////////////////////////////////////////////////////////////////
	float	DistToLine2(const CVec3 &Start, const CVec3 &Stop) const
	{
		CVec3	P(*this);
		P.ProjectToLineSeg(Start, Stop);

		return Dist2(P);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Translation & Rotation (2D)
    ////////////////////////////////////////////////////////////////////////////////////
	void	RotatePoint(const CVec3 &Angle, const CVec3 &Origin);
	void	Reposition(const CVec3 &Translation, float RotationDegrees=0.0);



    ////////////////////////////////////////////////////////////////////////////////////
	// Area Of The Parallel Pipid (2D)
	//
	// Given two more points, this function calculates the area of the parallel pipid
	// formed.
	//
	// Note: This function CAN return a negative "area" if (this) is above or right of
	// (A) and (B)...  We do not take the abs because the sign of the "area" is needed
	// for the left right test (see below)
	//
	//
	//               ___---( ... )
	//        (A)---/        /
	//        /             /
	//       /             /
	//      /             /
	//     /      ___---(B)
	//  (this)---/
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	AreaParallelPipid(const CVec3 &A, const CVec3 &B) const
	{
		return ((A.v[0]*B.v[1] - A.v[1]*B.v[0]) +
			    (B.v[0]*  v[1] -   v[0]*B.v[1]) +
				(  v[0]*A.v[1] - A.v[0]*  v[1]));
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Area Of The Triangle (2D)
	//
	// Given two more points, this function calculates the area of the triangle formed.
	//
	//        (A)
	//        /  \__
	//       /      \__
	//      /          \_
	//     /      ___---(B)
	//  (this)---/
	//
    ////////////////////////////////////////////////////////////////////////////////////
	float	AreaTriange(const CVec3 &A, const CVec3 &B) const
	{
		return (AreaParallelPipid(A, B) * 0.5f);
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// The Left Right Test (2D)
	//
	// Given a line segment (Start->End) and a tolerance for *right on*, this function
	// evaluates which side the point is of the line.  (Side_Left in this example)
	//
	//
	//
	//          (this)        ___---/(End)
	//                 ___---/
	//          ___---/
	//  (Start)/
	//
    ////////////////////////////////////////////////////////////////////////////////////
	ESide	LRTest(const CVec3 &Start, const CVec3 &End, float Tolerance=0.0) const
	{
		float Area = AreaParallelPipid(Start, End);
		if (Area>Tolerance)
		{
			return Side_Left;
		}
		if (Area<(Tolerance*-1))
		{
			return Side_Right;
		}
		return Side_None;

	}

	////////////////////////////////////////////////////////////////////////////////////
	// Point In Circumscribed Circle  (True/False)
	//
	//  Returns true if the given point is within the circumscribed
	//  circle of the given ABC Triangle:
	//         _____
	//        /   B \
	//      /   /   \ \
	//     |  /      \ |
	//     |A---------C|
	//      \    Pt   /
	//       \_______/
	//
	////////////////////////////////////////////////////////////////////////////////////
	bool	PtInCircle(const CVec3 &A, const CVec3 &B, const CVec3 &C) const;


	////////////////////////////////////////////////////////////////////////////////////
	// Point In Standard Circle  (True/False)
	//
	//  Returns true if the given point is within the Circle
	//         _____
	//        /     \
	//      /         \
	//     |   Circle  |
	//     |           |
	//      \    Pt   /
	//       \_______/
	//
	////////////////////////////////////////////////////////////////////////////////////
	bool	PtInCircle(const CVec3 &Circle, float Radius) const;

	////////////////////////////////////////////////////////////////////////////////////
	// Line Intersects Circle  (True/False)
	//
	//  r	- Radius Of The Circle
	//  A	- Start Of Line Segment
	//  B	- End Of Line Segment
	//
	//  P	- Projected Position Of Origin Onto Line AB
	//
	//
	//            (Stop)
	//              /
	//            /
	//         (P)
	//        /   \      \
	//      /   (this)-r->|
	//    /              /
	// (Start)
	//
	////////////////////////////////////////////////////////////////////////////////////
	bool	LineInCircle(const CVec3 &Start, const CVec3 &Stop, float Radius);
	bool	LineInCircle(const CVec3 &Start, const CVec3 &Stop, float Radius, CVec3 &PointOnLine);



    ////////////////////////////////////////////////////////////////////////////////////
	// String Operations
    ////////////////////////////////////////////////////////////////////////////////////
	void	FromStr(const char *s);
	void	ToStr(char* s) const;


    ////////////////////////////////////////////////////////////////////////////////////
	// Debug Routines
    ////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	bool	IsFinite();
	bool	IsInitialized();
#endif


    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////

public:
	float v[3];
	static const CVec3 mX;
	static const CVec3 mY;
	static const CVec3 mZ;
	static const CVec3 mZero;
};



//};
