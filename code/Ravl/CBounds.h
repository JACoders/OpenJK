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
// Vector Library
// --------------
//
//
//
//
// NOTES:
// 05/31/02 - CREATED
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RAVL_BOUNDS_INC)
#define RAVL_BOUNDS_INC
// namespace ravl
//{

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "CVec.h"

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define RAVL_BB_EMPTY_MIN (1.234567E30f)  // Empty Value
#define RAVL_BB_EMPTY_MAX (-1.234567E30f) // Empty Value

////////////////////////////////////////////////////////////////////////////////////////
// Enums And Typedefs
////////////////////////////////////////////////////////////////////////////////////////
typedef unsigned char TPlanes;

////////////////////////////////////////////////////////////////////////////////////////
// The Bounds Trace
////////////////////////////////////////////////////////////////////////////////////////
class CBTrace {
public:
  CBTrace(const CVec3 &Start, const CVec3 &Stop)
      : mStart(Start), mStop(Stop), mAim(Stop), mRange(0), mRangeMax(0),
        mPoint(Stop) {
    mAim -= Start;
    mLength = mAim.Norm();
  }

  CBTrace &operator=(const CBTrace &T) {
    mStart = (T.mStart);
    mStop = (T.mStop);
    mAim = (T.mAim);
    mRange = (T.mRange);
    mRangeMax = (T.mRangeMax);
    mPoint = (T.mPoint);
    return (*this);
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Setup Values, Do Not Change
  ////////////////////////////////////////////////////////////////////////////////////
public:
  CVec3 mStart;
  CVec3 mStop;
  CVec3 mAim;
  float mLength;

  ////////////////////////////////////////////////////////////////////////////////////
  // Results
  ////////////////////////////////////////////////////////////////////////////////////
public:
  float mRange;
  float mRangeMax;
  CVec3 mPoint;
  CVec3 mNormal;
};

////////////////////////////////////////////////////////////////////////////////////////
// The Bounding Box Class
////////////////////////////////////////////////////////////////////////////////////////
class CBBox {
public:
  ////////////////////////////////////////////////////////////////////////////////////
  // Constructors
  ////////////////////////////////////////////////////////////////////////////////////
  CBBox() {
    mMin.Set(RAVL_BB_EMPTY_MIN);
    mMax.Set(RAVL_BB_EMPTY_MAX);
  }
  CBBox(float Radius) {
    mMin.Set(-Radius);
    mMax.Set(Radius);
  }
  CBBox(const CVec3 &t) {
    mMin = t;
    mMax = t;
  }
  CBBox(const CVec3 &min, const CVec3 &max) {
    mMin = min;
    mMax = max;
  }
  CBBox(const CBBox &t) {
    mMin = t.mMin;
    mMax = t.mMax;
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Initializers
  ////////////////////////////////////////////////////////////////////////////////////
  void Set(const CVec3 &min, const CVec3 &max) {
    mMin = min;
    mMax = max;
    Validate();
  }
  void Clear() {
    mMin.Set(RAVL_BB_EMPTY_MIN);
    mMax.Set(RAVL_BB_EMPTY_MAX);
  }
  void AddPoint(const CVec3 &p) {
    mMin.Min(p);
    mMax.Max(p);
    Validate();
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Accessors
  ////////////////////////////////////////////////////////////////////////////////////
  bool IsEmpty() const { return (mMin[0] == RAVL_BB_EMPTY_MIN); }
  CVec3 Center() const { return (mMin + mMax) * 0.5; }

  ////////////////////////////////////////////////////////////////////////////////////
  // Translation, Rotation, Expansion
  ////////////////////////////////////////////////////////////////////////////////////
  void Translate(const CVec3 &f) {
    mMin += f;
    mMax += f;
  }
  void Expand(float x) {
    mMin -= x;
    mMax += x;
  }
  void Expand(const CVec3 &f) {
    mMin -= f;
    mMax += f;
  }
  //	void	ThroughMatrix(const CBBox &from, const CMatrix4 &mat);

  ////////////////////////////////////////////////////////////////////////////////////
  // Volumetric & Area Operations
  ////////////////////////////////////////////////////////////////////////////////////
  float Volume() const {
    return (mMax[0] - mMin[0]) * (mMax[1] - mMin[1]) * (mMax[2] - mMin[2]);
  }
  float AxisSize(int axis) const { return (mMax[axis] - mMin[axis]); }
  float LargestAxisSize() const;
  float DistanceEstimate(const CVec3 &p) const; // Manhattan Distance
  float
  AreaEstimate(const CVec3 &p) const; // Manhattan Distance * LargestAxisSize()

  ////////////////////////////////////////////////////////////////////////////////////
  // Set Operations
  ////////////////////////////////////////////////////////////////////////////////////
  void Intersect(const CBBox &b2);
  void Union(const CBBox &b2);

  ////////////////////////////////////////////////////////////////////////////////////
  // Tests
  ////////////////////////////////////////////////////////////////////////////////////
  ESide InOutTest(const CVec3 &p) const;
  ESide InOutTest(const CVec3 &p, float tolout, float tolin) const;
  bool BoxTouchTest(const CBBox &b2, float tolout = 0.0f) const;
  bool SphereTouchTest(const CVec3 &c, float rad) const;
  TPlanes PlaneFlags(const CVec3 &p);

  ////////////////////////////////////////////////////////////////////////////////////
  // Hit Tests
  ////////////////////////////////////////////////////////////////////////////////////
  bool HitTest(CBTrace &Tr) const;

  ////////////////////////////////////////////////////////////////////////////////////
  // String Operations
  ////////////////////////////////////////////////////////////////////////////////////
  void FromStr(const char *s);
  void ToStr(char *s);

  ////////////////////////////////////////////////////////////////////////////////////
  // Debug Operations
  ////////////////////////////////////////////////////////////////////////////////////
  void Validate();

  ////////////////////////////////////////////////////////////////////////////////////
  // Data
  ////////////////////////////////////////////////////////////////////////////////////
public:
  CVec3 mMin;
  CVec3 mMax;
};

//};
#endif
