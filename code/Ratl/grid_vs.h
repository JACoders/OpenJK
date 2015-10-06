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
// Grid
// ----
// There are two versions of the Grid class.  Simply, they apply a discreet function
// mapping from a n dimensional space to a linear aray.
//
//
//
//
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_GRID_VS_INC)
#define RATL_GRID_VS_INC


////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif
#if !defined(RATL_ARRAY_VS)
	#include "array_vs.h"
#endif
namespace ratl
{



////////////////////////////////////////////////////////////////////////////////////////
// The 2D Grid Class
////////////////////////////////////////////////////////////////////////////////////////
template <class T, int XSIZE_MAX, int YSIZE_MAX>
class grid2_vs : public ratl_base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructor
    ////////////////////////////////////////////////////////////////////////////////////
	grid2_vs()
	{
		clear();
	}

	enum
	{
		RANGE_NULL = 12345,
	};


    ////////////////////////////////////////////////////////////////////////////////////
	// Assignment Operator
    ////////////////////////////////////////////////////////////////////////////////////
	grid2_vs&	operator=(const grid2_vs& other)
	{
		mData = other.mData;
		for (int i=0; i<2; i++)
		{
			mSize[i] = other.mSize[i];
			mMins[i] = other.mMins[i];
			mMaxs[i] = other.mMaxs[i];
			mScale[i] = other.mScale[i];
		}
		return (*this);
	}

	void		set_size(int xSize, int ySize)
	{
		if (xSize<XSIZE_MAX)
		{
			mSize[0] = xSize;
		}
		if (ySize<YSIZE_MAX)
		{
			mSize[1] = ySize;
		}
	}

	void		snap_scale()
	{
		mScale[0] = (float)((int)(mScale[0]));
		mScale[1] = (float)((int)(mScale[1]));
	}

	void		get_size(int& xSize, int& ySize)
	{
		xSize = mSize[0];
		ySize = mSize[1];
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// Clear
    ////////////////////////////////////////////////////////////////////////////////////
	void		clear()
	{
		mSize[0] = XSIZE_MAX;
		mSize[1] = YSIZE_MAX;
		mData.clear();
		for (int i=0; i<2; i++)
		{
			mMins[i] = RANGE_NULL;
			mMaxs[i] = RANGE_NULL;
			mScale[i] = 0.0f;
		}
	}


    ////////////////////////////////////////////////////////////////////////////////////
	// Initialize The Entire Grid To A Value
    ////////////////////////////////////////////////////////////////////////////////////
	void		init(const T& val)
	{
		for (int i=0; i<(XSIZE_MAX*YSIZE_MAX); i++)
		{
			mData[i] = val;
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Copy The Bounds Of Another Grid
    ////////////////////////////////////////////////////////////////////////////////////
	void		copy_bounds(const grid2_vs& other)
	{
		for (int i=0; i<2; i++)
		{
			mSize[i] = other.mSize[i];
			mMins[i] = other.mMins[i];
			mMaxs[i] = other.mMaxs[i];
			mScale[i] = other.mScale[i];
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	T&			get(const int x, const int y)
	{
		assert(x>=0 && y>=0 && x<mSize[0] && y<mSize[1]);
		return mData[(x + y*XSIZE_MAX)];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Accessor
    ////////////////////////////////////////////////////////////////////////////////////
	T&			get(float x, float y)
	{
		assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		truncate_position_to_bounds(x, y);

		int xint = (int)( (x-mMins[0]) / mScale[0] );
		int yint = (int)( (y-mMins[1]) / mScale[1] );

		assert(xint>=0 && yint>=0 && xint<mSize[0] && yint<mSize[1]);
		return mData[(xint + yint*XSIZE_MAX)];
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Convert The Scaled Coordinates To A Grid Coordinate
    ////////////////////////////////////////////////////////////////////////////////////
	void		get_cell_coords(float x, float y, int& xint, int& yint)
	{
		assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		truncate_position_to_bounds(x, y);

		xint = (int)( (x-mMins[0]) / mScale[0] );
		yint = (int)( (y-mMins[1]) / mScale[1] );

		assert(xint>=0 && yint>=0 && xint<mSize[0] && yint<mSize[1]);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Expand
	//
	// NOTE:  This MUST be at least a 2 dimensional point
    ////////////////////////////////////////////////////////////////////////////////////
	void		expand_bounds(float xReal, float yReal)
	{
		float	point[2] = {xReal, yReal};
		for (int i=0; i<2; i++)
		{
			if (point[i]<mMins[i] || mMins[i]==RANGE_NULL)
			{
				mMins[i] = point[i];
			}
			if (point[i]>mMaxs[i] || mMaxs[i]==RANGE_NULL)
			{
				mMaxs[i] = point[i];
			}
		}
		assert(mSize[0]>0 && mSize[1]>0);

		mScale[0] = ((mMaxs[0] - mMins[0])/mSize[0]);
		mScale[1] = ((mMaxs[1] - mMins[1])/mSize[1]);
	}

    ////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void		truncate_position_to_bounds(float& xReal, float& yReal)
	{
		if (xReal<mMins[0])
		{
			xReal = mMins[0];
		}
		if (xReal>(mMaxs[0]-1.0f))
		{
			xReal = mMaxs[0]-1.0f;
		}
		if (yReal<mMins[1])
		{
			yReal = mMins[1];
		}
		if (yReal>(mMaxs[1]-1.0f))
		{
			yReal = mMaxs[1]-1.0f;
		}
	}

    ////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	void		get_cell_position(int x, int y, float& xReal, float& yReal)
	{
	//	assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		xReal = (x * mScale[0]) + mMins[0] + (mScale[0] * 0.5f);
		yReal = (y * mScale[1]) + mMins[1] + (mScale[1] * 0.5f);
	}
	void		get_cell_upperleft(int x, int y, float& xReal, float& yReal)
	{
	//	assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		xReal = (x * mScale[0]) + mMins[0];
		yReal = (y * mScale[1]) + mMins[1];
	}
	void		get_cell_lowerright(int x, int y, float& xReal, float& yReal)
	{
	//	assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		xReal = (x * mScale[0]) + mMins[0] + (mScale[0]);
		yReal = (y * mScale[1]) + mMins[1] + (mScale[1]);
	}
	void		scale_by_largest_axis(float& dist)
	{
		assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		if (mScale[0]>mScale[1])
		{
			dist /= mScale[0];
		}
		else
		{
			dist /= mScale[1];
		}
	}




    ////////////////////////////////////////////////////////////////////////////////////
	// Data
    ////////////////////////////////////////////////////////////////////////////////////
private:
	array_vs<T, XSIZE_MAX*YSIZE_MAX>	mData;

	int							mSize[2];
	float						mMins[2];
	float						mMaxs[2];
	float						mScale[2];




public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Raw Get - For The Iterator Dereference Function
    ////////////////////////////////////////////////////////////////////////////////////
	T&			rawGet(int Loc)
	{
		assert(Loc>=0 && Loc<XSIZE_MAX*YSIZE_MAX);
		return mData[Loc];
	}



    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class iterator
	{
	public:

		// Constructors
		//--------------
		iterator()														{}
		iterator(grid2_vs* p, int t)	: 	mOwner(p), mLoc(t)			{}

		// Assignment Operator
		//---------------------
		void		operator= (const iterator &t)						{mOwner=t.mOwner;	mLoc=t.mLoc;}

		// Equality & Inequality Operators
		//---------------------------------
		bool		operator!=(const iterator &t)						{return (mLoc!=t.mLoc);}
		bool		operator==(const iterator &t)						{return (mLoc==t.mLoc);}

		// Dereference Operator
		//----------------------
		T&			operator* ()										{return (mOwner->rawGet(mLoc));}

		// Inc Operator
		//--------------
		void		operator++(int)										{mLoc++;}


		// Row & Col Offsets
		//-------------------
		void		offsetRows(int num)									{mLoc += (YSIZE_MAX*num);}
		void		offsetCols(int num)									{mLoc += (num);}


		// Return True If On Frist Column Of A Row
		//-----------------------------------------
		bool		onColZero()
		{
			return (mLoc%XSIZE_MAX)==0;
		}

		// Evaluate The XY Position Of This Iterator
		//-------------------------------------------
		void		position(int& X, int& Y)
		{
			Y = mLoc / XSIZE_MAX;
			X = mLoc - (Y*XSIZE_MAX);
		}

	private:
		int					mLoc;
		grid2_vs*			mOwner;
	};

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator Begin
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin(int x=0, int y=0)
	{
		assert(x>=0 && y>=0 && x<mSize[0] && y<mSize[1]);

		return iterator(this, (x + y*XSIZE_MAX));
	}

    ////////////////////////////////////////////////////////////////////////////////////
	// Iterator Begin (scaled position, use mins and maxs to calc real position)
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	begin(float xReal, float yReal)
	{
		assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		truncate_position_to_bounds(xReal, yReal);

		int x = (int)( (xReal-mMins[0]) / mScale[0] );
		int y = (int)( (yReal-mMins[1]) / mScale[1] );

		return begin(x,y);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Iterator End
    ////////////////////////////////////////////////////////////////////////////////////
	iterator	end()
	{
		return iterator(this, (XSIZE_MAX*YSIZE_MAX));
	}








    ////////////////////////////////////////////////////////////////////////////////////
	// Ranged Iterator
    ////////////////////////////////////////////////////////////////////////////////////
	class riterator
	{
	public:

		// Constructors
		//--------------
		riterator()
		{}
		riterator(grid2_vs* p, int Range, int SX, int SY)	:
			mOwner(p)
		{
			int		Start[2] = {SX, SY};
			int		Bounds[2] = {XSIZE_MAX-1, YSIZE_MAX-1};

			for (int i=0; i<2; i++)
			{
				mMins[i] = Start[i] - Range;
				mMaxs[i] = Start[i] + Range;

				if (mMins[i]<0)
				{
					mMins[i] = 0;
				}
				if (mMaxs[i] > Bounds[i])
				{
					mMaxs[i] = Bounds[i];
				}

				mLoc[i] = mMins[i];
			}
		}

		// Assignment Operator
		//---------------------
		void		operator= (const riterator &t)
		{
			mOwner		= t.mOwner;
			for (int i=0; i<2; i++)
			{
				mMins[i] = t.mMins[i];
				mMaxs[i] = t.mMaxs[i];
				mLoc[i]  = t.mLoc[i];
			}
		}

		// Equality & Inequality Operators
		//---------------------------------
		bool		operator!=(const riterator &t)
		{
			return (mLoc[0]!=t.mLoc[0] || mLoc[1]!=t.mLoc[1]);
		}
		bool		operator==(const riterator &t)
		{
			return (mLoc[0]==t.mLoc[0] && mLoc[1]==t.mLoc[1]);
		}

		// Dereference Operator
		//----------------------
		T&			operator* ()
		{
			return (mOwner->get(mLoc[0], mLoc[1]));
		}

		// Inc Operator
		//--------------
		void		operator++(int)
		{
			if (mLoc[1] <= mMaxs[1])
			{
				mLoc[0]++;
				if (mLoc[0]>(mMaxs[0]))
				{
					mLoc[0] = mMins[0];
					mLoc[1]++;
				}
			}
		}

		bool		at_end()
		{
			return (mLoc[1]>mMaxs[1]);
		}


		// Return True If On Frist Column Of A Row
		//-----------------------------------------
		bool		onColZero()
		{
			return (mLoc[0]==mMins[0]);
		}

		// Evaluate The XY Position Of This Iterator
		//-------------------------------------------
		void		position(int& X, int& Y)
		{
			Y = mLoc[1];
			X = mLoc[0];
		}

	private:
		int						mMins[2];
		int						mMaxs[2];
		int						mLoc[2];
		grid2_vs*				mOwner;
	};

    ////////////////////////////////////////////////////////////////////////////////////
	// Ranged Iterator Begin (x and y are the center of the range)
    ////////////////////////////////////////////////////////////////////////////////////
	riterator	rangeBegin(int range, int x, int y)
	{
		assert(x>=0 && y>=0 && x<XSIZE_MAX && y<YSIZE_MAX);
		return riterator(this, range, x, y);
	}

	////////////////////////////////////////////////////////////////////////////////////
	//
    ////////////////////////////////////////////////////////////////////////////////////
	riterator	rangeBegin(int range, float xReal, float yReal)
	{
		float	position[2] = {xReal, yReal};
		assert(mScale[0]!=0.0f && mScale[1]!=0.0f);
		truncate_position_to_bounds(xReal, yReal);
		int x = ( (position[0]-mMins[0]) / mScale[0] );
		int y = ( (position[1]-mMins[1]) / mScale[1] );

		assert(x>=0 && y>=0 && x<XSIZE_MAX && y<YSIZE_MAX);
		return riterator(this, range, x, y);
	}
};

}
#endif



