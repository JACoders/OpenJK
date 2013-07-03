/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#pragma once
#if !defined(CM_RANDOMTERRAIN_H_INC)
#define CM_RANDOMTERRAIN_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including cm_randomterrain.h")
#endif

//class CPathInfo;

#define SPLINE_MERGE_SIZE	3
#define	CIRCLE_STAMP_SIZE	128


class CPathInfo
{
private:
	vec4_t		*mPoints, *mWork;
	vec_t		*mWeights;
	int			mNumPoints;
	float		mInc;
	float		mDepth, mBreadth;
	float		mDeviation;
	byte		mCircleStamp[CIRCLE_STAMP_SIZE][CIRCLE_STAMP_SIZE];

	void		CreateCircle(void);
	void		Stamp(int x, int y, int size, int depth, unsigned char *Data, int DataWidth, int DataHeight);

public:
	CPathInfo(CCMLandScape *landscape, int numPoints, float bx, float by, float ex, float ey, 
		float minWidth, float maxWidth, float depth, float deviation, float breadth,
		CPathInfo *Connected, unsigned CreationFlags);
	~CPathInfo(void);

	int		GetNumPoints(void) { return mNumPoints; }
	float	*GetPoint(int index) { return mPoints[index]; }
	float	GetWidth(int index) { return mPoints[index][3]; }

	void	GetInfo(float PercentInto, vec4_t Coord, vec4_t Vector);
	void	DrawPath(unsigned char *Data, int DataWidth, int DataHeight );
};


const int			MAX_RANDOM_PATHS	= 30;

// Path Creation Flags
#define		PATH_CREATION_CONNECT_FRONT		0x00000001



class CRandomTerrain
{
private:

	class CCMLandScape	*mLandScape;
	int					mWidth;
	int					mHeight;
	int					mArea;
	int					mBorder;
	byte				*mGrid;
	CPathInfo			*mPaths[MAX_RANDOM_PATHS];

public:
	CRandomTerrain(void);
	~CRandomTerrain(void);

	CCMLandScape		*GetLandScape(void) { return mLandScape; }
	const vec3pair_t	&GetBounds(void) const { return mLandScape->GetBounds(); }
	void				rand_seed(int seed) { mLandScape->rand_seed(seed); }
	int					get_rand_seed(void) { return mLandScape->get_rand_seed();}
	float				flrand(float min, float max) { return mLandScape->flrand(min, max); }
	int					irand(int min, int max) { return mLandScape->irand(min, max); }

	void	Init(class CCMLandScape *landscape, byte *data, int width, int height);
	void	Shutdown(void);
	bool	CreatePath(int PathID, int ConnectedID, unsigned CreationFlags, int numPoints, 
						float bx, float by, float ex, float ey, 
						float minWidth, float maxWidth, float depth, float deviation, float breadth );
	bool	GetPathInfo(int PathNum, float PercentInto, vec2_t Coord, vec2_t Vector);
	void	ParseGenerate(const char *GenerateFile);
	void	Smooth ( void );
	void	Generate(int symmetric);
	void	ClearPaths(void);
};

unsigned RMG_CreateSeed(char *TextSeed);

#endif // CM_RANDOM_H_INC
