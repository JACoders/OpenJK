/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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
// Vector Library
////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include <math.h>
#include <stdio.h>
#include <float.h>
#include "CVec.h"

//using namespace ravl;



////////////////////////////////////////////////////////////////////////////////////////
// Static Class Member Initialization
////////////////////////////////////////////////////////////////////////////////////////
const CVec4 CVec4::mX(1.f,		0.f,	0.f,	0.f);
const CVec4 CVec4::mY(0.f,		1.f,	0.f,	0.f);
const CVec4 CVec4::mZ(0.f,		0.f,	1.f,	0.f);
const CVec4 CVec4::mW(0.f,		0.f,	0.f,	1.f);
const CVec4 CVec4::mZero(0.f,	0.f,	0.f,	0.f);




////////////////////////////////////////////////////////////////////////////////////////
// Length
////////////////////////////////////////////////////////////////////////////////////////
float	CVec4::Len() const
{
	return sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+v[3]*v[3]);
}


////////////////////////////////////////////////////////////////////////////////////////
// Distance To Other Point
////////////////////////////////////////////////////////////////////////////////////////
float	CVec4::Dist(const CVec4& t) const
{
	return sqrtf(
		(t.v[0]-v[0])*(t.v[0]-v[0]) + 
		(t.v[1]-v[1])*(t.v[1]-v[1]) + 
		(t.v[2]-v[2])*(t.v[2]-v[2]) + 
		(t.v[3]-v[3])*(t.v[3]-v[3]));
}

////////////////////////////////////////////////////////////////////////////////////////
// Normalize
////////////////////////////////////////////////////////////////////////////////////////
float	CVec4::Norm()
{
	float	L = Len();
	(*this)/=L;
	return L;
} 

////////////////////////////////////////////////////////////////////////////////////////
// Safe Normalize
//  Do Not Normalize If Length Is Too Small
////////////////////////////////////////////////////////////////////////////////////////
float	CVec4::SafeNorm()
{
	float d=this->Len();
	if (d>1E-10) 
	{
		(*this)/=d;
		return d;
	} 
	(*this)=0.0f; 
	return 0;
} 

////////////////////////////////////////////////////////////////////////////////////////
// Angular Normalize 
//  All floats Exist(-180, +180)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::AngleNorm()
{
	v[0]= fmodf(v[0],360.0f);
	if (v[0]<-180.f)	v[0]+=360.0f;
	if (v[0]>180.f)		v[0]-=360.0f;

	v[1]= fmodf(v[1],360.0f);
	if (v[1]<-180.f)	v[1]+=360.0f;
	if (v[1]>180.f)		v[1]-=360.0f;

	v[2]= fmodf(v[2],360.0f);
	if (v[2]<-180.f)	v[2]+=360.0f;
	if (v[2]>180.f)		v[2]-=360.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
// Perpendicular Vector
//
// This implimentation is a bit slow, needs some optimization work...
//
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::Perp()
{
	float rlen,tlen;
	CVec4 r,t;
	r = (*this);
	r.Cross(mX);
	rlen=r.Len();
	t = (*this);
	t.Cross(mY);
	tlen=t.Len();
	if (tlen>rlen)
	{
		r=t;
		rlen=tlen;
	}
	t = (*this);
	t.Cross(mZ);
	tlen=t.Len();
	if (tlen>rlen)
	{
		r=t;
		rlen=tlen;
	}
	(*this) = r;
}

////////////////////////////////////////////////////////////////////////////////////////
// Find Largest Element (Ignores 4th component for now)
////////////////////////////////////////////////////////////////////////////////////////
int		CVec4::MaxElementIndex() const
{
	if (fabs(v[0])>fabs(v[1]) && fabs(v[0])>fabs(v[2]))
	{
		return 0;
	}
	if (fabs(v[1])>fabs(v[2]))
	{
		return 1;
	}
	return 2;	
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert To {Pitch, Yaw}   (DEGREES)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::VecToAng()
{
	float	yaw;
	float	pitch;
	
	if (!v[1] && !v[0]) 
	{
		yaw		= 0;
		pitch	= (v[2]>0) ? (90.0f):(270.0f);
	}
	else 
	{
		// Calculate Yaw
		//---------------
		if (v[0]) 
		{
			yaw = (RAVL_VEC_RADTODEG(atan2f(v[1], v[0])));
			if (yaw<0) 
			{
				yaw += 360;
			}
		}
		else 
		{
			yaw	= (v[1]>0) ? (90.0f):(270.0f);
		}

		// Calculate Pitch
		//-----------------
		float forward	= (sqrtf(v[0]*v[0] + v[1]*v[1]));
		pitch			= (RAVL_VEC_RADTODEG(atan2f(v[2], forward)));
		if (pitch<0)
		{
			pitch += 360;
		}
	}

	// Copy Over Current Vector
	//--------------------------
	v[0]	= -pitch;
	v[1]	= yaw;
	v[2]	= 0;
	v[3]	= 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw}   (DEGREES)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::AngToVec()
{
	float		angle;
	float		sp, sy, cp, cy;

	angle	= yaw() * (RAVL_VEC_DEGTORADCONST);
	sy		= sinf(angle);
	cy		= cosf(angle);
	angle	= pitch() * (RAVL_VEC_DEGTORADCONST);
	sp		= sinf(angle);
	cp		= cosf(angle);

	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;
	v[3] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw, Roll}   (DEGREES)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::AngToVec(CVec4& Right, CVec4& Up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle	= yaw() * (RAVL_VEC_DEGTORADCONST);
	sy		= sinf(angle);
	cy		= cosf(angle);
	angle	= pitch() * (RAVL_VEC_DEGTORADCONST);
	sp		= sinf(angle);
	cp		= cosf(angle);
	angle	= roll() * (RAVL_VEC_DEGTORADCONST);
	sr		= sinf(angle);
	cr		= cosf(angle);

	// Forward Vector Is Stored Here
	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;
	v[3] = 0;

	// Calculate Right
	Right.v[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
	Right.v[1] = (-1 * sr * sp * sy + -1 * cr * cy);
	Right.v[2] = -1 * sr * cp;
	Right.v[3] = 0;

	// Calculate Up
	Up.v[0] = (cr * sp * cy + -sr * -sy);
	Up.v[1] = (cr * sp * sy + -sr * cy);
	Up.v[2] = cr * cp;
	Up.v[3] = 0;
}




///////////////////////////////////////////////////////////////////////////////////////
// Convert To {Pitch, Yaw}   (RADIANS)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::VecToAngRad()
{
	float	yaw;
	float	pitch;
	
	if (!v[1] && !v[0]) 
	{
		yaw		= 0;
		pitch	= (v[2]>0) ? (RAVL_VEC_PI*0.5f):(RAVL_VEC_PI*1.5f);
	}
	else 
	{
		// Calculate Yaw
		//---------------
		if (v[0]) 
		{
			yaw = (atan2f(v[1], v[0]));
		}
		else 
		{
			yaw	= (v[1]>0) ? (RAVL_VEC_PI*0.5f):(RAVL_VEC_PI*1.5f);
		}

		// Calculate Pitch
		//-----------------
		float forward	= (sqrtf(v[0]*v[0] + v[1]*v[1]));
		pitch			= (atan2f(v[2], forward));
	}

	// Copy Over Current Vector
	//--------------------------
	v[0]	= -pitch;
	v[1]	= yaw;
	v[2]	= 0;
	v[3]	= 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw}   (RADIANS)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::AngToVecRad()
{
	float		sp, sy, cp, cy;

	sy		= sinf(yaw());
	cy		= cosf(yaw());
	sp		= sinf(pitch());
	cp		= cosf(pitch());

	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;
	v[3] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw, Roll}   (RADIANS)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::AngToVecRad(CVec4& Right, CVec4& Up)
{
	float		sr, sp, sy, cr, cp, cy;

	sy		= sinf(yaw());
	cy		= cosf(yaw());
	sp		= sinf(pitch());
	cp		= cosf(pitch());
	sr		= sinf(roll());
	cr		= cosf(roll());

	// Forward Vector Is Stored Here
	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;
	v[3] = 0;

	// Calculate Right
	Right.v[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
	Right.v[1] = (-1 * sr * sp * sy + -1 * cr * cy);
	Right.v[2] = -1 * sr * cp;
	Right.v[3] = 0;

	// Calculate Up
	Up.v[0] = (cr * sp * cy + -sr * -sy);
	Up.v[1] = (cr * sp * sy + -sr * cy);
	Up.v[2] = cr * cp;
	Up.v[3] = 0;
}


////////////////////////////////////////////////////////////////////////////////////////
// Convert To Radians
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::ToRadians()
{
	v[0] = RAVL_VEC_DEGTORAD(v[0]);
	v[1] = RAVL_VEC_DEGTORAD(v[1]);
	v[2] = RAVL_VEC_DEGTORAD(v[2]);
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert To Degrees
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::ToDegrees()
{
	v[0] = RAVL_VEC_RADTODEG(v[0]);
	v[1] = RAVL_VEC_RADTODEG(v[1]);
	v[2] = RAVL_VEC_RADTODEG(v[2]);
}





////////////////////////////////////////////////////////////////////////////////////////
// Copy Values From String
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::FromStr(const char *s)
{
//	assert(s && s[0]);
	sscanf(s, "(%f %f %f %f)", &v[0], &v[1], &v[2], &v[3]);
}

////////////////////////////////////////////////////////////////////////////////////////
// Write Values To A String
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::ToStr(char* s) const
{
//	assert(s);
	sprintf(s, "(%3.3f %3.3f %3.3f %3.3f)", v[0], v[1], v[2], v[3]);
}



#ifdef _DEBUG

////////////////////////////////////////////////////////////////////////////////////////
// Make Sure Entire Vector Has Valid Numbers
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec4::IsFinite()
{
#if defined(_MSC_VER) 
	return 	(_finite(v[0]) && _finite(v[1]) && _finite(v[2]) && _finite(v[3]));
#else
	return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// Make Sure Vector Has Been Initialized
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec4::IsInitialized()
{
	return (v[0]!=RAVL_VEC_UDF && v[1]!=RAVL_VEC_UDF && v[2]!=RAVL_VEC_UDF && v[3]!=RAVL_VEC_UDF);
}

#endif 




////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec4::PtInCircle(const CVec4 &A, const CVec4 &B, const CVec4 &C) const
{
	float  vol;
	float  ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz;
	float  bxdx, bydy, bzdz, cxdx, cydy, czdz;

	float  tolerance=0.00000005f;

	ax = A.v[0];
	ay = A.v[1];
	az = ax*ax + ay*ay;
	bx = B.v[0];
	by = B.v[1];
	bz = bx*bx + by*by;
	cx = C.v[0];
	cy = C.v[1];
	cz = cx*cx + cy*cy;
	dx = v[0];
	dy = v[1];
	dz = dx*dx + dy*dy;

	bxdx=bx-dx;
	bydy=by-dy;
	bzdz=bz-dz;
	cxdx=cx-dx;
	cydy=cy-dy;
	czdz=cz-dz;
	vol = (az-dz)*(bxdx*cydy-bydy*cxdx) + (ay-dy)*(bzdz*cxdx-bxdx*czdz) + (ax-dx)*(bydy*czdz-bzdz*cydy);

	if      ( vol >    tolerance)  return  true;
	else if ( vol < -1*tolerance)  return  false;
	else                           return  false;
}

////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec4::PtInCircle(const CVec4 &Circle, float Radius) const
{
	return (Dist2(Circle)<(Radius*Radius));
}





////////////////////////////////////////////////////////////////////////////////////////
// Line Intersects Circle  (True/False)
//
//  r	- Radius Of The Circle
//  A	- Start Of Line Segment
//  B	- End Of Line Segment
//
//  P	- Projected Position Of Origin Onto Line AB
//
//
//               B
//             /
//           /
//         P
//       /   \      \
//     /     this-r->|
//   /              /
// A
//
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec4::LineInCircle(const CVec4 &A, const CVec4 &B, float r, CVec4 &P)
{
	P = (*this);
	float	Scale = P.ProjectToLine(A, B);

	// If The Projected Position Is Not On The Line Segment,
	// Check If It Is Within Radius Of Endpoints A and B.
	//-------------------------------------------------------
	if (Scale<0 || Scale>1)
	{
		return (PtInCircle(A, r) || PtInCircle(B, r));
	}

	// Otherwise, Check To See If P Is Within The Radius Of This Circle
	//------------------------------------------------------------------
	return (PtInCircle(P, r));
}

////////////////////////////////////////////////////////////////////////////////////////
// Same As Test Above, Just Don't Bother Returning P
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec4::LineInCircle(const CVec4 &A, const CVec4 &B, float r)
{
	CVec4	P(*this);
	float	Scale = P.ProjectToLine(A, B);

	// If The Projected Position Is Not On The Line Segment,
	// Check If It Is Within Radius Of Endpoints A and B.
	//-------------------------------------------------------
	if (Scale<0 || Scale>1)
	{
		return (PtInCircle(A, r) || PtInCircle(B, r));
	}

	// Otherwise, Check To See If P Is Within The Radius Of This Circle
	//------------------------------------------------------------------
	return (PtInCircle(P, r));
}


////////////////////////////////////////////////////////////////////////////////////////
// Rotate
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::RotatePoint(const CVec4 &, const CVec4 &)
{
	// TO DO: Use Matrix Code To Rotate
}

////////////////////////////////////////////////////////////////////////////////////////
// Reposition
////////////////////////////////////////////////////////////////////////////////////////
void	CVec4::Reposition(const CVec4 &Translation, float RotationDegrees)
{
	// Apply Any Rotation First
	//--------------------------
	if (RotationDegrees)
	{
		CVec4 Old(*this);
		float Rotation = RAVL_VEC_DEGTORAD(RotationDegrees);
		v[0] = Old.v[0]*cosf(Rotation) - Old.v[1]*sinf(Rotation);
		v[1] = Old.v[0]*sinf(Rotation) + Old.v[1]*cosf(Rotation);
	}

	// Now Apply Translation
	//-----------------------
	(*this) += Translation;
}













////////////////////////////////////////////////////////////////////////////////////////
// Static Class Member Initialization
////////////////////////////////////////////////////////////////////////////////////////
const CVec3 CVec3::mX(1.f,		0.f,	0.f);
const CVec3 CVec3::mY(0.f,		1.f,	0.f);
const CVec3 CVec3::mZ(0.f,		0.f,	1.f);
const CVec3 CVec3::mZero(0.f,	0.f,	0.f);




////////////////////////////////////////////////////////////////////////////////////////
// Length
////////////////////////////////////////////////////////////////////////////////////////
float	CVec3::Len() const
{
	return sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}


////////////////////////////////////////////////////////////////////////////////////////
// Distance To Other Point
////////////////////////////////////////////////////////////////////////////////////////
float	CVec3::Dist(const CVec3& t) const
{
	return sqrtf(
		(t.v[0]-v[0])*(t.v[0]-v[0]) + 
		(t.v[1]-v[1])*(t.v[1]-v[1]) + 
		(t.v[2]-v[2])*(t.v[2]-v[2]));
}

////////////////////////////////////////////////////////////////////////////////////////
// Normalize
////////////////////////////////////////////////////////////////////////////////////////
float	CVec3::Norm()
{
	float	L = Len();
	(*this)/=L;
	return L;
} 

////////////////////////////////////////////////////////////////////////////////////////
// Safe Normalize
//  Do Not Normalize If Length Is Too Small
////////////////////////////////////////////////////////////////////////////////////////
float	CVec3::SafeNorm()
{
	float d=this->Len();
	if (d>1E-10) 
	{
		(*this)/=d;
		return d;
	} 
	(*this)=0.0f; 
	return 0;
} 

////////////////////////////////////////////////////////////////////////////////////////
// Angular Normalize 
//  All floats Exist(-180, +180)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::AngleNorm()
{
	v[0]= fmodf(v[0],360.0f);
	if (v[0]<-180.f)	v[0]+=360.0f;
	if (v[0]>180.f)		v[0]-=360.0f;

	v[1]= fmodf(v[1],360.0f);
	if (v[1]<-180.f)	v[1]+=360.0f;
	if (v[1]>180.f)		v[1]-=360.0f;

	v[2]= fmodf(v[2],360.0f);
	if (v[2]<-180.f)	v[2]+=360.0f;
	if (v[2]>180.f)		v[2]-=360.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
// Angular Normalize 
//  All floats Exist(-180, +180)
////////////////////////////////////////////////////////////////////////////////////////
float	CVec3::Truncate(float maxlen)
{
	float len=this->Len();
	if (len>maxlen && len>1E-10)
	{
		len = maxlen / len;
		v[0] *= len;
		v[1] *= len;
		v[2] *= len;

		return maxlen;
	}
	return len;
}

////////////////////////////////////////////////////////////////////////////////////////
// Perpendicular Vector
//
// This implimentation is a bit slow, needs some optimization work...
//
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::Perp()
{
	float rlen,tlen;
	CVec3 r,t;
	r = (*this);
	r.Cross(mX);
	rlen=r.Len();
	t = (*this);
	t.Cross(mY);
	tlen=t.Len();
	if (tlen>rlen)
	{
		r=t;
		rlen=tlen;
	}
	t = (*this);
	t.Cross(mZ);
	tlen=t.Len();
	if (tlen>rlen)
	{
		r=t;
		rlen=tlen;
	}
	(*this) = r;
}

////////////////////////////////////////////////////////////////////////////////////////
// Find Largest Element (Ignores 4th component for now)
////////////////////////////////////////////////////////////////////////////////////////
int		CVec3::MaxElementIndex() const
{
	if (fabs(v[0])>fabs(v[1]) && fabs(v[0])>fabs(v[2]))
	{
		return 0;
	}
	if (fabs(v[1])>fabs(v[2]))
	{
		return 1;
	}
	return 2;	
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert To {Pitch, Yaw}   (DEGREES)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::VecToAng()
{
	float	yaw;
	float	pitch;
	
	if (!v[1] && !v[0]) 
	{
		yaw		= 0;
		pitch	= (v[2]>0) ? (90.0f):(270.0f);
	}
	else 
	{
		// Calculate Yaw
		//---------------
		if (v[0]) 
		{
			yaw = (RAVL_VEC_RADTODEG(atan2f(v[1], v[0])));
			if (yaw<0) 
			{
				yaw += 360;
			}
		}
		else 
		{
			yaw	= (v[1]>0) ? (90.0f):(270.0f);
		}

		// Calculate Pitch
		//-----------------
		float forward	= (sqrtf(v[0]*v[0] + v[1]*v[1]));
		pitch			= (RAVL_VEC_RADTODEG(atan2f(v[2], forward)));
		if (pitch<0)
		{
			pitch += 360;
		}
	}

	// Copy Over Current Vector
	//--------------------------
	v[0]	= -pitch;
	v[1]	= yaw;
	v[2]	= 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw}   (DEGREES)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::AngToVec()
{
	float		angle;
	float		sp, sy, cp, cy;

	angle	= yaw() * (RAVL_VEC_DEGTORADCONST);
	sy		= sinf(angle);
	cy		= cosf(angle);
	angle	= pitch() * (RAVL_VEC_DEGTORADCONST);
	sp		= sinf(angle);
	cp		= cosf(angle);

	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw, Roll}   (DEGREES)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::AngToVec(CVec3& Right, CVec3& Up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle	= yaw() * (RAVL_VEC_DEGTORADCONST);
	sy		= sinf(angle);
	cy		= cosf(angle);
	angle	= pitch() * (RAVL_VEC_DEGTORADCONST);
	sp		= sinf(angle);
	cp		= cosf(angle);
	angle	= roll() * (RAVL_VEC_DEGTORADCONST);
	sr		= sinf(angle);
	cr		= cosf(angle);

	// Forward Vector Is Stored Here
	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;

	// Calculate Right
	Right.v[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
	Right.v[1] = (-1 * sr * sp * sy + -1 * cr * cy);
	Right.v[2] = -1 * sr * cp;

	// Calculate Up
	Up.v[0] = (cr * sp * cy + -sr * -sy);
	Up.v[1] = (cr * sp * sy + -sr * cy);
	Up.v[2] = cr * cp;
}




///////////////////////////////////////////////////////////////////////////////////////
// Convert To {Pitch, Yaw}   (RADIANS)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::VecToAngRad()
{
	float	yaw;
	float	pitch;
	
	if (!v[1] && !v[0]) 
	{
		yaw		= 0;
		pitch	= (v[2]>0) ? (RAVL_VEC_PI*0.5f):(RAVL_VEC_PI*1.5f);
	}
	else 
	{
		// Calculate Yaw
		//---------------
		if (v[0]) 
		{
			yaw = (atan2f(v[1], v[0]));
		}
		else 
		{
			yaw	= (v[1]>0) ? (RAVL_VEC_PI*0.5f):(RAVL_VEC_PI*1.5f);
		}

		// Calculate Pitch
		//-----------------
		float forward	= (sqrtf(v[0]*v[0] + v[1]*v[1]));
		pitch			= (atan2f(v[2], forward));
	}

	// Copy Over Current Vector
	//--------------------------
	v[0]	= -pitch;
	v[1]	= yaw;
	v[2]	= 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw}   (RADIANS)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::AngToVecRad()
{
	float		sp, sy, cp, cy;

	sy		= sinf(yaw());
	cy		= cosf(yaw());
	sp		= sinf(pitch());
	cp		= cosf(pitch());

	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert From {Picth, Yaw, Roll}   (RADIANS)
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::AngToVecRad(CVec3& Right, CVec3& Up)
{
	float		sr, sp, sy, cr, cp, cy;

	sy		= sinf(yaw());
	cy		= cosf(yaw());
	sp		= sinf(pitch());
	cp		= cosf(pitch());
	sr		= sinf(roll());
	cr		= cosf(roll());

	// Forward Vector Is Stored Here
	v[0] = cp * cy;
	v[1] = cp * sy;
	v[2] = -sp;

	// Calculate Right
	Right.v[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
	Right.v[1] = (-1 * sr * sp * sy + -1 * cr * cy);
	Right.v[2] = -1 * sr * cp;

	// Calculate Up
	Up.v[0] = (cr * sp * cy + -sr * -sy);
	Up.v[1] = (cr * sp * sy + -sr * cy);
	Up.v[2] = cr * cp;
}


////////////////////////////////////////////////////////////////////////////////////////
// Convert To Radians
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::ToRadians()
{
	v[0] = RAVL_VEC_DEGTORAD(v[0]);
	v[1] = RAVL_VEC_DEGTORAD(v[1]);
	v[2] = RAVL_VEC_DEGTORAD(v[2]);
}

////////////////////////////////////////////////////////////////////////////////////////
// Convert To Degrees
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::ToDegrees()
{
	v[0] = RAVL_VEC_RADTODEG(v[0]);
	v[1] = RAVL_VEC_RADTODEG(v[1]);
	v[2] = RAVL_VEC_RADTODEG(v[2]);
}





////////////////////////////////////////////////////////////////////////////////////////
// Copy Values From String
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::FromStr(const char *s)
{
	assert(s && s[0]);
	sscanf(s, "(%f %f %f)", &v[0], &v[1], &v[2]);
}

////////////////////////////////////////////////////////////////////////////////////////
// Write Values To A String
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::ToStr(char* s) const
{
	assert(s);
	sprintf(s, "(%3.3f %3.3f %3.3f)", v[0], v[1], v[2]);
}



#ifdef _DEBUG

////////////////////////////////////////////////////////////////////////////////////////
// Make Sure Entire Vector Has Valid Numbers
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec3::IsFinite()
{
#if defined(_MSC_VER) 
		return 	(_finite(v[0]) && _finite(v[1]) && _finite(v[2]));
#else
		return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// Make Sure Vector Has Been Initialized
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec3::IsInitialized()
{
	return (v[0]!=RAVL_VEC_UDF && v[1]!=RAVL_VEC_UDF && v[2]!=RAVL_VEC_UDF);
}

#endif 




////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec3::PtInCircle(const CVec3 &A, const CVec3 &B, const CVec3 &C) const
{
	float  vol;
	float  ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz;
	float  bxdx, bydy, bzdz, cxdx, cydy, czdz;

	float  tolerance=0.00000005f;

	ax = A.v[0];
	ay = A.v[1];
	az = ax*ax + ay*ay;
	bx = B.v[0];
	by = B.v[1];
	bz = bx*bx + by*by;
	cx = C.v[0];
	cy = C.v[1];
	cz = cx*cx + cy*cy;
	dx = v[0];
	dy = v[1];
	dz = dx*dx + dy*dy;

	bxdx=bx-dx;
	bydy=by-dy;
	bzdz=bz-dz;
	cxdx=cx-dx;
	cydy=cy-dy;
	czdz=cz-dz;
	vol = (az-dz)*(bxdx*cydy-bydy*cxdx) + (ay-dy)*(bzdz*cxdx-bxdx*czdz) + (ax-dx)*(bydy*czdz-bzdz*cydy);

	if      ( vol >    tolerance)  return  true;
	else if ( vol < -1*tolerance)  return  false;
	else                           return  false;
}

////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec3::PtInCircle(const CVec3 &Circle, float Radius) const
{
	return (Dist2(Circle)<(Radius*Radius));
}





////////////////////////////////////////////////////////////////////////////////////////
// Line Intersects Circle  (True/False)
//
//  r	- Radius Of The Circle
//  A	- Start Of Line Segment
//  B	- End Of Line Segment
//
//  P	- Projected Position Of Origin Onto Line AB
//
//
//               B
//             /
//           /
//         P
//       /   \      \
//     /     this-r->|
//   /              /
// A
//
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec3::LineInCircle(const CVec3 &A, const CVec3 &B, float r, CVec3 &P)
{
	P = (*this);
	float	Scale = P.ProjectToLine(A, B);

	// If The Projected Position Is Not On The Line Segment,
	// Check If It Is Within Radius Of Endpoints A and B.
	//-------------------------------------------------------
	if (Scale<0 || Scale>1)
	{
		return (PtInCircle(A, r) || PtInCircle(B, r));
	}

	// Otherwise, Check To See If P Is Within The Radius Of This Circle
	//------------------------------------------------------------------
	return (PtInCircle(P, r));
}

////////////////////////////////////////////////////////////////////////////////////////
// Same As Test Above, Just Don't Bother Returning P
////////////////////////////////////////////////////////////////////////////////////////
bool	CVec3::LineInCircle(const CVec3 &A, const CVec3 &B, float r)
{
	CVec3	P(*this);
	float	Scale = P.ProjectToLine(A, B);

	// If The Projected Position Is Not On The Line Segment,
	// Check If It Is Within Radius Of Endpoints A and B.
	//-------------------------------------------------------
	if (Scale<0 || Scale>1)
	{
		return (PtInCircle(A, r) || PtInCircle(B, r));
	}

	// Otherwise, Check To See If P Is Within The Radius Of This Circle
	//------------------------------------------------------------------
	return (PtInCircle(P, r));
}


////////////////////////////////////////////////////////////////////////////////////////
// Rotate
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::RotatePoint(const CVec3 &, const CVec3 &)
{
	// TO DO: Use Matrix Code To Rotate
}

////////////////////////////////////////////////////////////////////////////////////////
// Reposition
////////////////////////////////////////////////////////////////////////////////////////
void	CVec3::Reposition(const CVec3 &Translation, float RotationDegrees)
{
	// Apply Any Rotation First
	//--------------------------
	if (RotationDegrees)
	{
		CVec3 Old(*this);
		float Rotation = RAVL_VEC_DEGTORAD(RotationDegrees);
		v[0] = Old.v[0]*cosf(Rotation) - Old.v[1]*sinf(Rotation);
		v[1] = Old.v[0]*sinf(Rotation) + Old.v[1]*cosf(Rotation);
	}

	// Now Apply Translation
	//-----------------------
	(*this) += Translation;
}

