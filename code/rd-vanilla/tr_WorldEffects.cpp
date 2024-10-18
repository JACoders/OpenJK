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
// RAVEN SOFTWARE - STAR WARS: JK II
//  (c) 2002 Activision
//
// World Effects
//
//
////////////////////////////////////////////////////////////////////////////////////////
#include "../server/exe_headers.h"

////////////////////////////////////////////////////////////////////////////////////////
// Externs & Fwd Decl.
////////////////////////////////////////////////////////////////////////////////////////
extern void			SetViewportAndScissor( void );

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "tr_local.h"
#include "tr_WorldEffects.h"
#include "../Ravl/CVec.h"
#include "../Ratl/vector_vs.h"
#include "../Ratl/bits_vs.h"

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define GLS_ALPHA				(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
#define	MAX_WIND_ZONES			12
#define MAX_WEATHER_ZONES		50	// so we can more zones that are smaller
#define	MAX_PUFF_SYSTEMS		2
#define	MAX_PARTICLE_CLOUDS		5
#define POINTCACHE_CELL_SIZE	32.0f

////////////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////////////
float		mMillisecondsElapsed = 0;
float		mSecondsElapsed = 0;
bool		mFrozen = false;

CVec3		mGlobalWindVelocity;
CVec3		mGlobalWindDirection;
float		mGlobalWindSpeed;
int			mParticlesRendered;



////////////////////////////////////////////////////////////////////////////////////////
// Handy Functions
////////////////////////////////////////////////////////////////////////////////////////
inline void		VectorMA( vec3_t vecAdd, const float scale, const vec3_t vecScale)
{
	vecAdd[0] += (scale * vecScale[0]);
	vecAdd[1] += (scale * vecScale[1]);
	vecAdd[2] += (scale * vecScale[2]);
}

inline void VectorFloor(vec3_t in)
{
	in[0] = floorf(in[0]);
	in[1] = floorf(in[1]);
	in[2] = floorf(in[2]);
}

inline void VectorCeil(vec3_t in)
{
	in[0] = ceilf(in[0]);
	in[1] = ceilf(in[1]);
	in[2] = ceilf(in[2]);
}

inline float	FloatRand(void)
{
	return ((float)rand() / (float)RAND_MAX);
}

inline float	fast_flrand(float min, float max)
{
	//return min + (max - min) * flrand;
	return Q_flrand(min, max); //fixme?
}

inline	void	SnapFloatToGrid(float& f, int GridSize)
{
	f = (int)(f);

	bool	fNeg		= (f<0);
	if (fNeg)
	{
		f *= -1;		// Temporarly make it positive
	}

	int		Offset		= ((int)(f) % (int)(GridSize));
	int		OffsetAbs	= abs(Offset);
	if (OffsetAbs>(GridSize/2))
	{
		Offset = (GridSize - OffsetAbs) * -1;
	}

	f -= Offset;

	if (fNeg)
	{
		f *= -1;		// Put It Back To Negative
	}

	f = (int)(f);

	assert(((int)(f)%(int)(GridSize)) == 0);
}

inline	void	SnapVectorToGrid(CVec3& Vec, int GridSize)
{
	SnapFloatToGrid(Vec[0], GridSize);
	SnapFloatToGrid(Vec[1], GridSize);
	SnapFloatToGrid(Vec[2], GridSize);
}





////////////////////////////////////////////////////////////////////////////////////////
// Range Structures
////////////////////////////////////////////////////////////////////////////////////////
struct	SVecRange
{
	CVec3	mMins;
	CVec3	mMaxs;

	inline void	Clear()
	{
		mMins.Clear();
		mMaxs.Clear();
	}

	inline void Pick(CVec3& V)
	{
		V[0] = Q_flrand(mMins[0], mMaxs[0]);
		V[1] = Q_flrand(mMins[1], mMaxs[1]);
		V[2] = Q_flrand(mMins[2], mMaxs[2]);
	}
	inline void Wrap(CVec3& V)
	{
		if (V[0]<=mMins[0])
		{
			if ((mMins[0]-V[0])>500)
			{
				Pick(V);
				return;
			}
			V[0] = mMaxs[0] - 10.0f;
		}
		if (V[0]>=mMaxs[0])
		{
			if ((V[0]-mMaxs[0])>500)
			{
				Pick(V);
				return;
			}
			V[0] = mMins[0] + 10.0f;
		}

		if (V[1]<=mMins[1])
		{
			if ((mMins[1]-V[1])>500)
			{
				Pick(V);
				return;
			}
			V[1] = mMaxs[1] - 10.0f;
		}
		if (V[1]>=mMaxs[1])
		{
			if ((V[1]-mMaxs[1])>500)
			{
				Pick(V);
				return;
			}
			V[1] = mMins[1] + 10.0f;
		}

		if (V[2]<=mMins[2])
		{
			if ((mMins[2]-V[2])>500)
			{
				Pick(V);
				return;
			}
			V[2] = mMaxs[2] - 10.0f;
		}
		if (V[2]>=mMaxs[2])
		{
			if ((V[2]-mMaxs[2])>500)
			{
				Pick(V);
				return;
			}
			V[2] = mMins[2] + 10.0f;
		}
	}

	inline bool In(const CVec3& V)
	{
		return (V>mMins && V<mMaxs);
	}
};

struct	SFloatRange
{
	float	mMin;
	float	mMax;

	inline void	Clear()
	{
		mMin = 0;
		mMax = 0;
	}
	inline void Pick(float& V)
	{
		V = Q_flrand(mMin, mMax);
	}
	inline bool In(const float& V)
	{
		return (V>mMin && V<mMax);
	}
};

struct	SIntRange
{
	int	mMin;
	int	mMax;

	inline void	Clear()
	{
		mMin = 0;
		mMax = 0;
	}
	inline void Pick(int& V)
	{
		V = Q_irand(mMin, mMax);
	}
	inline bool In(const int& V)
	{
		return (V>mMin && V<mMax);
	}
};





////////////////////////////////////////////////////////////////////////////////////////
// The Particle Class
////////////////////////////////////////////////////////////////////////////////////////
class	WFXParticle
{
public:
	enum
	{
		FLAG_RENDER = 0,

		FLAG_FADEIN,
		FLAG_FADEOUT,
		FLAG_RESPAWN,

		FLAG_MAX
	};
	typedef		ratl::bits_vs<FLAG_MAX>		TFlags;

	float	mAlpha;
	TFlags	mFlags;
	CVec3	mPosition;
	CVec3	mVelocity;
	float	mMass;			// A higher number will more greatly resist force and result in greater gravity
};





////////////////////////////////////////////////////////////////////////////////////////
// The Wind
////////////////////////////////////////////////////////////////////////////////////////
class	CWindZone
{
public:
	bool		mGlobal;
	SVecRange	mRBounds;
	SVecRange	mRVelocity;
	SIntRange	mRDuration;
	SIntRange	mRDeadTime;
	float		mMaxDeltaVelocityPerUpdate;
	float		mChanceOfDeadTime;

	CVec3		mCurrentVelocity;
	CVec3		mTargetVelocity;
	int			mTargetVelocityTimeRemaining;


public:
	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	void		Initialize()
	{
		mRBounds.Clear();
		mGlobal						= true;

		mRVelocity.mMins			= -1500.0f;
		mRVelocity.mMins[2]			= -10.0f;
		mRVelocity.mMaxs			= 1500.0f;
		mRVelocity.mMaxs[2]			= 10.0f;

		mMaxDeltaVelocityPerUpdate	= 10.0f;

		mRDuration.mMin				= 1000;
		mRDuration.mMax				= 2000;

		mChanceOfDeadTime			= 0.3f;
		mRDeadTime.mMin				= 1000;
		mRDeadTime.mMax				= 3000;

		mCurrentVelocity.Clear();
		mTargetVelocity.Clear();
		mTargetVelocityTimeRemaining = 0;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Update - Changes wind when current target velocity expires
	////////////////////////////////////////////////////////////////////////////////////
	void		Update()
	{
		if (mTargetVelocityTimeRemaining==0)
		{
			if (FloatRand()<mChanceOfDeadTime)
			{
				mRDeadTime.Pick(mTargetVelocityTimeRemaining);
				mTargetVelocity.Clear();
			}
			else
			{
				mRDuration.Pick(mTargetVelocityTimeRemaining);
				mRVelocity.Pick(mTargetVelocity);
			}
		}
		else if (mTargetVelocityTimeRemaining!=-1)
		{
			mTargetVelocityTimeRemaining--;

			CVec3	DeltaVelocity(mTargetVelocity - mCurrentVelocity);
			float	DeltaVelocityLen = VectorNormalize(DeltaVelocity.v);
			if (DeltaVelocityLen > mMaxDeltaVelocityPerUpdate)
			{
				DeltaVelocityLen = mMaxDeltaVelocityPerUpdate;
			}
			DeltaVelocity *= (DeltaVelocityLen);
			mCurrentVelocity += DeltaVelocity;
		}
	}
};
ratl::vector_vs<CWindZone, MAX_WIND_ZONES>		mWindZones;
ratl::vector_vs<CWindZone*, MAX_WIND_ZONES>		mLocalWindZones;

bool R_GetWindVector(vec3_t windVector, vec3_t atpoint)
{
	VectorCopy(mGlobalWindDirection.v, windVector);
	if (atpoint && mLocalWindZones.size())
	{
		for (int curLocalWindZone=0; curLocalWindZone<mLocalWindZones.size(); curLocalWindZone++)
		{
			if (mLocalWindZones[curLocalWindZone]->mRBounds.In(atpoint))
			{
				VectorAdd(windVector, mLocalWindZones[curLocalWindZone]->mCurrentVelocity.v, windVector);
			}
		}
		VectorNormalize(windVector);
	}
	return true;
}

bool R_GetWindSpeed(float &windSpeed, vec3_t atpoint)
{
	windSpeed = mGlobalWindSpeed;
	if (atpoint && mLocalWindZones.size())
	{
		for (int curLocalWindZone=0; curLocalWindZone<mLocalWindZones.size(); curLocalWindZone++)
		{
			if (mLocalWindZones[curLocalWindZone]->mRBounds.In(atpoint))
			{
				windSpeed += VectorLength(mLocalWindZones[curLocalWindZone]->mCurrentVelocity.v);
			}
		}
	}
	return true;
}

bool R_GetWindGusting(vec3_t atpoint)
{
	float windSpeed;
	R_GetWindSpeed(windSpeed, atpoint);
	return (windSpeed>1000.0f);
}

////////////////////////////////////////////////////////////////////////////////////////
// Outside Point Cache
////////////////////////////////////////////////////////////////////////////////////////
class COutside
{
#define COUTSIDE_STRUCT_VERSION 1	// you MUST increase this any time you change any binary (fields) inside this class, or cahced files will fuck up
public:
	////////////////////////////////////////////////////////////////////////////////////
	//Global Public Outside Variables
	////////////////////////////////////////////////////////////////////////////////////

	bool			mOutsideShake;
	float			mOutsidePain;

	CVec3			mFogColor;
	int				mFogColorInt;
	bool			mFogColorTempActive;

private:
	////////////////////////////////////////////////////////////////////////////////////
	// The Outside Cache
	////////////////////////////////////////////////////////////////////////////////////
	bool			mCacheInit;			// Has It Been Cached?

	struct SWeatherZone
	{
		static bool	mMarkedOutside;
		uint32_t	*mPointCache;			// malloc block ptr

		int			miPointCacheByteSize;	// size of block
		SVecRange	mExtents;
		SVecRange	mSize;
		int			mWidth;
		int			mHeight;
		int			mDepth;

		void WriteToDisk( fileHandle_t f )
		{
			ri.FS_Write(&mMarkedOutside,sizeof(mMarkedOutside),f);
			ri.FS_Write( mPointCache, miPointCacheByteSize, f );
		}

		void ReadFromDisk( fileHandle_t f )
		{
			ri.FS_Read(&mMarkedOutside,sizeof(mMarkedOutside),f);
			ri.FS_Read( mPointCache, miPointCacheByteSize, f);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Convert To Cell
		////////////////////////////////////////////////////////////////////////////////////
		inline	void	ConvertToCell(const CVec3& pos, int& x, int& y, int& z, int& bit)
		{
			x = (int)((pos[0] / POINTCACHE_CELL_SIZE) - mSize.mMins[0]);
			y = (int)((pos[1] / POINTCACHE_CELL_SIZE) - mSize.mMins[1]);
			z = (int)((pos[2] / POINTCACHE_CELL_SIZE) - mSize.mMins[2]);

			bit = (z & 31);
			z >>= 5;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// CellOutside - Test to see if a given cell is outside
		////////////////////////////////////////////////////////////////////////////////////
		inline	bool	CellOutside(int x, int y, int z, int bit)
		{
			if ((x < 0 || x >= mWidth) || (y < 0 || y >= mHeight) || (z < 0 || z >= mDepth) || (bit < 0 || bit >= 32))
			{
				return !(mMarkedOutside);
			}
			return (mMarkedOutside==(!!(mPointCache[((z * mWidth * mHeight) + (y * mWidth) + x)]&(1 << bit))));
		}
	};
	ratl::vector_vs<SWeatherZone, MAX_WEATHER_ZONES>	mWeatherZones;


private:
	////////////////////////////////////////////////////////////////////////////////////
	// Iteration Variables
	////////////////////////////////////////////////////////////////////////////////////
	int				mWCells;
	int				mHCells;

	int				mXCell;
	int				mYCell;
	int				mZBit;

	int				mXMax;
	int				mYMax;
	int				mZMax;


private:


	////////////////////////////////////////////////////////////////////////////////////
	// Contents Outside
	////////////////////////////////////////////////////////////////////////////////////
	inline	bool	ContentsOutside(int contents)
	{
		if (contents&CONTENTS_WATER || contents&CONTENTS_SOLID)
		{
			return false;
		}
		if (mCacheInit)
		{
			if (SWeatherZone::mMarkedOutside)
			{
				return (!!(contents&CONTENTS_OUTSIDE));
			}
			return (!(contents&CONTENTS_INSIDE));
		}
		return !!(contents&CONTENTS_OUTSIDE);
	}




public:
	////////////////////////////////////////////////////////////////////////////////////
	// Constructor - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	void Reset()
	{
		mOutsideShake = false;
		mOutsidePain = 0.0;
		mCacheInit = false;
		SWeatherZone::mMarkedOutside = false;

		mFogColor.Clear();
		mFogColorInt = 0;
		mFogColorTempActive = false;

		for (int wz=0; wz<mWeatherZones.size(); wz++)
		{
			R_Free(mWeatherZones[wz].mPointCache);
			mWeatherZones[wz].mPointCache = 0;
			mWeatherZones[wz].miPointCacheByteSize = 0;	// not really necessary because of .clear() below, but keeps things together in case stuff changes
		}
		mWeatherZones.clear();
	}

	COutside()
	{
		Reset();
	}
	~COutside()
	{
		Reset();
	}

	bool			Initialized()
	{
		return mCacheInit;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// AddWeatherZone - Will add a zone of mins and maxes
	////////////////////////////////////////////////////////////////////////////////////
	void			AddWeatherZone(vec3_t mins, vec3_t maxs)
	{
		if (mCacheInit)
		{
			return;
		}
		if (!mWeatherZones.full())
		{
			SWeatherZone&	Wz = mWeatherZones.push_back();
			Wz.mExtents.mMins = mins;
			Wz.mExtents.mMaxs = maxs;

			SnapVectorToGrid(Wz.mExtents.mMins, POINTCACHE_CELL_SIZE);
			SnapVectorToGrid(Wz.mExtents.mMaxs, POINTCACHE_CELL_SIZE);

			Wz.mSize.mMins = Wz.mExtents.mMins;
			Wz.mSize.mMaxs = Wz.mExtents.mMaxs;

			Wz.mSize.mMins /= POINTCACHE_CELL_SIZE;
			Wz.mSize.mMaxs /= POINTCACHE_CELL_SIZE;
			Wz.mWidth		=  (int)(Wz.mSize.mMaxs[0] - Wz.mSize.mMins[0]);
			Wz.mHeight		=  (int)(Wz.mSize.mMaxs[1] - Wz.mSize.mMins[1]);
			Wz.mDepth		= ((int)(Wz.mSize.mMaxs[2] - Wz.mSize.mMins[2]) + 31) >> 5;

			Wz.miPointCacheByteSize = (Wz.mWidth * Wz.mHeight * Wz.mDepth) * sizeof(uint32_t);
			Wz.mPointCache  = (uint32_t *)R_Malloc( Wz.miPointCacheByteSize, TAG_POINTCACHE, qtrue );
		}
		else
		{
			assert("MaxWeatherZones Hit!"==0);
		}
	}

	const char *GenCachedWeatherFilename(void)
	{
		return va("maps/%s.weather", sv_mapname->string);
	}

	// weather file format...
	//
	struct WeatherFileHeader_t
	{
		int m_iVersion;
		int m_iChecksum;

		WeatherFileHeader_t()
		{
			m_iVersion			= COUTSIDE_STRUCT_VERSION;
			m_iChecksum			= sv_mapChecksum->integer;
		}
	};

	fileHandle_t WriteCachedWeatherFile( void )
	{
		fileHandle_t f = ri.FS_FOpenFileWrite( GenCachedWeatherFilename(), qtrue );
		if (f)
		{
			WeatherFileHeader_t WeatherFileHeader;

            ri.FS_Write(&WeatherFileHeader, sizeof(WeatherFileHeader), f);
			return f;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "(Unable to open weather file \"%s\" for writing!)\n",GenCachedWeatherFilename());
		}

		return 0;
	}

	// returns 0 for not-found or invalid file, else open handle to continue read from (which you then close yourself)...
	//
	fileHandle_t ReadCachedWeatherFile( void )
	{
		fileHandle_t f = 0;
		ri.FS_FOpenFileRead( GenCachedWeatherFilename(), &f, qfalse );
		if ( f )
		{
			// ok, it exists, but is it valid for this map?...
			//
			WeatherFileHeader_t WeatherFileHeaderForCompare;
			WeatherFileHeader_t WeatherFileHeaderFromDisk;

			ri.FS_Read(&WeatherFileHeaderFromDisk, sizeof(WeatherFileHeaderFromDisk), f);

			if (!memcmp(&WeatherFileHeaderForCompare, &WeatherFileHeaderFromDisk, sizeof(WeatherFileHeaderFromDisk)))
			{
				// go for it...
				//
				return f;
			}

            ri.Printf( PRINT_WARNING, "( Cached weather file \"%s\" out of date, regenerating... )\n",GenCachedWeatherFilename());
			ri.FS_FCloseFile( f );
		}
		else
		{
			ri.Printf( PRINT_WARNING, "( No cached weather file found, generating... )\n");
		}

		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Cache - Will Scan the World, Creating The Cache
	////////////////////////////////////////////////////////////////////////////////////
	void			Cache()
	{
		if (!tr.world || mCacheInit)
		{
			return;
		}

		// all this piece of code does really is fill in the bool "SWeatherZone::mMarkedOutside", plus the mPointCache[] for each zone,
		//	so we can diskload those. Maybe.
		fileHandle_t f = ReadCachedWeatherFile();
		if ( f )
		{
			for (int iZone=0; iZone<mWeatherZones.size(); iZone++)
			{
				SWeatherZone wz = mWeatherZones[iZone];
				wz.ReadFromDisk( f );
			}

			mCacheInit = true;
		}
		else
		{
			CVec3		CurPos;
			CVec3		Size;
			CVec3		Mins;
			int			x, y, z, q, zbase;
			bool		curPosOutside;
			uint32_t		contents;
			uint32_t		bit;


			// Record The Extents Of The World Incase No Other Weather Zones Exist
			//---------------------------------------------------------------------
			if (!mWeatherZones.size())
			{
				Com_Printf("WARNING: No Weather Zones Encountered\n");
				AddWeatherZone(tr.world->bmodels[0].bounds[0], tr.world->bmodels[0].bounds[1]);
			}

			f = WriteCachedWeatherFile();

			// Iterate Over All Weather Zones
			//--------------------------------
			for (int zone=0; zone<mWeatherZones.size(); zone++)
			{
				SWeatherZone	wz = mWeatherZones[zone];

				// Make Sure Point Contents Checks Occur At The CENTER Of The Cell
				//-----------------------------------------------------------------
				Mins = wz.mExtents.mMins;
				for (x=0; x<3; x++)
				{
					Mins[x] += (POINTCACHE_CELL_SIZE/2);
				}


				// Start Scanning
				//----------------
				for(z = 0; z < wz.mDepth; z++)
				{
					for(q = 0; q < 32; q++)
					{
						bit = (1 << q);
						zbase = (z << 5);

						for(x = 0; x < wz.mWidth; x++)
						{
							for(y = 0; y < wz.mHeight; y++)
							{
								CurPos[0] = x			* POINTCACHE_CELL_SIZE;
								CurPos[1] = y			* POINTCACHE_CELL_SIZE;
								CurPos[2] = (zbase + q)	* POINTCACHE_CELL_SIZE;
								CurPos	  += Mins;

								contents = ri.CM_PointContents(CurPos.v, 0);
								if (contents&CONTENTS_INSIDE || contents&CONTENTS_OUTSIDE)
								{
									curPosOutside = ((contents&CONTENTS_OUTSIDE)!=0);
									if (!mCacheInit)
									{
										mCacheInit = true;
										SWeatherZone::mMarkedOutside = curPosOutside;
									}
									else if (SWeatherZone::mMarkedOutside!=curPosOutside)
									{
										assert(0);
										Com_Error (ERR_DROP, "Weather Effect: Both Indoor and Outdoor brushs encountered in map.\n" );
										return;
									}

									// Mark The Point
									//----------------
									wz.mPointCache[((z * wz.mWidth * wz.mHeight) + (y * wz.mWidth) + x)] |= bit;
								}
							}// for (y)
						}// for (x)
					}// for (q)
				}// for (z)

				if (f)
				{
					mWeatherZones[ zone ].WriteToDisk( f );
				}
			}
		}

		if (f)
		{
			ri.FS_FCloseFile(f);
			f=0;	// not really necessary, but wtf.
		}

		// If no indoor or outdoor brushes were found
		//--------------------------------------------
		if (!mCacheInit)
		{
			mCacheInit = true;
			SWeatherZone::mMarkedOutside = false;		// Assume All Is Outside, Except Solid
		}
	}

public:
	////////////////////////////////////////////////////////////////////////////////////
	// PointOutside - Test to see if a given point is outside
	////////////////////////////////////////////////////////////////////////////////////
	inline	bool	PointOutside(const CVec3& pos)
	{
		if (!mCacheInit)
		{
			return ContentsOutside(ri.CM_PointContents(pos.v, 0));
		}
		for (int zone=0; zone<mWeatherZones.size(); zone++)
		{
			SWeatherZone	wz = mWeatherZones[zone];
			if (wz.mExtents.In(pos))
			{
				int		bit, x, y, z;
				wz.ConvertToCell(pos, x, y, z, bit);
				return wz.CellOutside(x, y, z, bit);
			}
		}
		return !(SWeatherZone::mMarkedOutside);

	}


	////////////////////////////////////////////////////////////////////////////////////
	// PointOutside - Test to see if a given bounded plane is outside
	////////////////////////////////////////////////////////////////////////////////////
	inline	bool	PointOutside(const CVec3& pos, float width, float height)
	{
		for (int zone=0; zone<mWeatherZones.size(); zone++)
		{
			SWeatherZone	wz = mWeatherZones[zone];
			if (wz.mExtents.In(pos))
			{
				int		bit, x, y, z;
				wz.ConvertToCell(pos, x, y, z, bit);
				if (width<POINTCACHE_CELL_SIZE || height<POINTCACHE_CELL_SIZE)
				{
 					return (wz.CellOutside(x, y, z, bit));
				}

				mWCells = ((int)width  / POINTCACHE_CELL_SIZE);
				mHCells = ((int)height / POINTCACHE_CELL_SIZE);

				mXMax = x + mWCells;
				mYMax = y + mWCells;
				mZMax = bit + mHCells;

				for (mXCell=x-mWCells; mXCell<=mXMax; mXCell++)
				{
					for (mYCell=y-mWCells; mYCell<=mYMax; mYCell++)
					{
						for (mZBit=bit-mHCells; mZBit<=mZMax; mZBit++)
						{
							if (!wz.CellOutside(mXCell, mYCell, z, mZBit))
							{
								return false;
							}
						}
					}
				}
				return true;
			}
		}
		return !(SWeatherZone::mMarkedOutside);
	}
};
COutside			mOutside;
bool				COutside::SWeatherZone::mMarkedOutside = false;


void R_AddWeatherZone(vec3_t mins, vec3_t maxs)
{
	mOutside.AddWeatherZone(mins, maxs);
}

bool R_IsOutside(vec3_t pos)
{
	return mOutside.PointOutside(pos);
}

bool R_IsShaking(vec3_t pos)
{
	return (mOutside.mOutsideShake && mOutside.PointOutside(pos));
}

float R_IsOutsideCausingPain(vec3_t pos)
{
	return (mOutside.mOutsidePain && mOutside.PointOutside(pos));
}

bool R_SetTempGlobalFogColor(vec3_t color)
{
	if (tr.world && tr.world->globalFog != -1)
	{
		// If Non Zero, Try To Set The Color
		//-----------------------------------
		if (color[0] || color[1] || color[2])
		{
			// Remember The Normal Fog Color
			//-------------------------------
			if (!mOutside.mFogColorTempActive)
			{
				mOutside.mFogColor				= tr.world->fogs[tr.world->globalFog].parms.color;
				mOutside.mFogColorInt			= tr.world->fogs[tr.world->globalFog].colorInt;
				mOutside.mFogColorTempActive	= true;
			}

			// Set The New One
			//-----------------
			tr.world->fogs[tr.world->globalFog].parms.color[0] = color[0];
			tr.world->fogs[tr.world->globalFog].parms.color[1] = color[1];
			tr.world->fogs[tr.world->globalFog].parms.color[2] = color[2];
			tr.world->fogs[tr.world->globalFog].colorInt = ColorBytes4 (
												color[0] * tr.identityLight,
												color[1] * tr.identityLight,
												color[2] * tr.identityLight,
												1.0 );
		}

		// If Unable TO Parse The Command Color Vector, Restore The Previous Fog Color
		//-----------------------------------------------------------------------------
		else if (mOutside.mFogColorTempActive)
		{
			mOutside.mFogColorTempActive = false;

			tr.world->fogs[tr.world->globalFog].parms.color[0] = mOutside.mFogColor[0];
			tr.world->fogs[tr.world->globalFog].parms.color[1] = mOutside.mFogColor[1];
			tr.world->fogs[tr.world->globalFog].parms.color[2] = mOutside.mFogColor[2];
			tr.world->fogs[tr.world->globalFog].colorInt	   = mOutside.mFogColorInt;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// Particle Cloud
////////////////////////////////////////////////////////////////////////////////////////
class	CParticleCloud
{
private:
	////////////////////////////////////////////////////////////////////////////////////
	// DYNAMIC MEMORY
	////////////////////////////////////////////////////////////////////////////////////
	image_t*	mImage;
	WFXParticle*	mParticles;

private:
	////////////////////////////////////////////////////////////////////////////////////
	// RUN TIME VARIANTS
	////////////////////////////////////////////////////////////////////////////////////
	float		mSpawnSpeed;
	CVec3		mSpawnPlaneNorm;
	CVec3		mSpawnPlaneRight;
	CVec3		mSpawnPlaneUp;
	SVecRange	mRange;

	CVec3		mCameraPosition;
	CVec3		mCameraForward;
	CVec3		mCameraLeft;
	CVec3		mCameraDown;
	CVec3		mCameraLeftPlusUp;
	CVec3		mCameraLeftMinusUp;


	int			mParticleCountRender;
	int			mGLModeEnum;

	bool		mPopulated;


public:
	////////////////////////////////////////////////////////////////////////////////////
	// CONSTANTS
	////////////////////////////////////////////////////////////////////////////////////
	bool		mOrientWithVelocity;
	float		mSpawnPlaneSize;
	float		mSpawnPlaneDistance;
	SVecRange	mSpawnRange;

	float		mGravity;			// How much gravity affects the velocity of a particle
	CVec4		mColor;				// RGBA color
	int			mVertexCount;		// 3 for triangle, 4 for quad, other numbers not supported

	float		mWidth;
	float		mHeight;

	int			mBlendMode;			// 0 = ALPHA, 1 = SRC->SRC
	int			mFilterMode;		// 0 = LINEAR, 1 = NEAREST

	float		mFade;				// How much to fade in and out 1.0 = instant, 0.01 = very slow

	SFloatRange	mRotation;
	float		mRotationDelta;
	float		mRotationDeltaTarget;
	float		mRotationCurrent;
	SIntRange	mRotationChangeTimer;
	int			mRotationChangeNext;

	SFloatRange	mMass;				// Determines how slowness to accelerate, higher number = slower
	float		mFrictionInverse;	// How much air friction does this particle have 1.0=none, 0.0=nomove

	int			mParticleCount;

	bool		mWaterParticles;




public:
	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Create Image, Particles, And Setup All Values
	////////////////////////////////////////////////////////////////////////////////////
	void	Initialize(int count, const char* texturePath, int VertexCount=4)
	{
		Reset();
		assert(mParticleCount==0 && mParticles==0);
		assert(mImage==0);

		// Create The Image
		//------------------
		mImage = R_FindImageFile(texturePath, qfalse, qfalse, qfalse, GL_CLAMP);
		if (!mImage)
		{
			Com_Error(ERR_DROP, "CParticleCloud: Could not texture %s", texturePath);
		}

		GL_Bind(mImage);



		// Create The Particles
		//----------------------
		mParticleCount	= count;
		mParticles		= new WFXParticle[mParticleCount];



		WFXParticle*	part=0;
		for (int particleNum=0; particleNum<mParticleCount; particleNum++)
		{
			part = &(mParticles[particleNum]);
			part->mPosition.Clear();
			part->mVelocity.Clear();
			part->mAlpha	= 0.0f;
			mMass.Pick(part->mMass);
		}

		mVertexCount = VertexCount;
		mGLModeEnum = (mVertexCount==3)?(GL_TRIANGLES):(GL_QUADS);
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Reset - Initializes all data to default values
	////////////////////////////////////////////////////////////////////////////////////
	void		Reset()
	{
		if (mImage)
		{
			// TODO: Free Image?
		}
		mImage				= 0;
		if (mParticleCount)
		{
			delete [] mParticles;
		}
		mParticleCount		= 0;
		mParticles			= 0;

		mPopulated			= 0;



		// These Are The Default Startup Values For Constant Data
		//========================================================
		mOrientWithVelocity = false;
		mWaterParticles		= false;

		mSpawnPlaneDistance	= 500;
		mSpawnPlaneSize		= 500;
		mSpawnRange.mMins	= -(mSpawnPlaneDistance*1.25f);
		mSpawnRange.mMaxs	=  (mSpawnPlaneDistance*1.25f);

		mGravity			= 300.0f;	// Units Per Second

		mColor				= 1.0f;

		mVertexCount		= 4;
		mWidth				= 1.0f;
		mHeight				= 1.0f;

		mBlendMode			= 0;
		mFilterMode			= 0;

		mFade				= 10.0f;

		mRotation.Clear();
		mRotationDelta		= 0.0f;
		mRotationDeltaTarget= 0.0f;
		mRotationCurrent	= 0.0f;
		mRotationChangeNext	= -1;
		mRotation.mMin		= -0.7f;
		mRotation.mMax		=  0.7f;
		mRotationChangeTimer.mMin = 500;
		mRotationChangeTimer.mMax = 2000;

		mMass.mMin			= 5.0f;
		mMass.mMax			= 10.0f;

		mFrictionInverse	= 0.7f;		// No Friction?
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Constructor - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	CParticleCloud()
	{
		mImage = 0;
		mParticleCount = 0;
		Reset();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	~CParticleCloud()
	{
		Reset();
	}


	////////////////////////////////////////////////////////////////////////////////////
	// UseSpawnPlane - Check To See If We Should Spawn On A Plane, Or Just Wrap The Box
	////////////////////////////////////////////////////////////////////////////////////
	inline bool	UseSpawnPlane()
	{
		return (mGravity!=0.0f);
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Update - Applies All Physics Forces To All Contained Particles
	////////////////////////////////////////////////////////////////////////////////////
	void		Update()
	{
		WFXParticle*	part=0;
		CVec3		partForce;
		CVec3		partMoved;
		CVec3		partToCamera;
		bool		partRendering;
		bool		partOutside;
		bool		partInRange;
		bool		partInView;
		int			particleNum;
		float		particleFade = (mFade * mSecondsElapsed);
		int			numLocalWindZones = mLocalWindZones.size();
		int			curLocalWindZone;


		// Compute Camera
		//----------------
		{
			mCameraPosition	= backEnd.viewParms.ori.origin;
			mCameraForward	= backEnd.viewParms.ori.axis[0];
			mCameraLeft		= backEnd.viewParms.ori.axis[1];
			mCameraDown		= backEnd.viewParms.ori.axis[2];

			if (mRotationChangeNext!=-1)
			{
				if (mRotationChangeNext==0)
				{
					mRotation.Pick(mRotationDeltaTarget);
					mRotationChangeTimer.Pick(mRotationChangeNext);
					if (mRotationChangeNext<=0)
					{
						mRotationChangeNext = 1;
					}
				}
				mRotationChangeNext--;

				float	RotationDeltaDifference = (mRotationDeltaTarget - mRotationDelta);
				if (fabsf(RotationDeltaDifference)>0.01)
				{
					mRotationDelta += RotationDeltaDifference;		// Blend To New Delta
				}
                mRotationCurrent += (mRotationDelta * mSecondsElapsed);
				float s = sinf(mRotationCurrent);
				float c = cosf(mRotationCurrent);

				CVec3	TempCamLeft(mCameraLeft);

				mCameraLeft *= (c * mWidth);
				mCameraLeft.ScaleAdd(mCameraDown, (s * mWidth * -1.0f));

				mCameraDown *= (c * mHeight);
				mCameraDown.ScaleAdd(TempCamLeft, (s * mHeight));
			}
			else
			{
				mCameraLeft		*= mWidth;
 				mCameraDown		*= mHeight;
			}
		}


		// Compute Global Force
		//----------------------
		CVec3		force;
		{
			force.Clear();

			// Apply Gravity
			//---------------
			force[2] = -1.0f * mGravity;

			// Apply Wind Velocity
			//---------------------
			force    += mGlobalWindVelocity;
		}


		// Update Range
		//--------------
		{
			mRange.mMins = mCameraPosition + mSpawnRange.mMins;
			mRange.mMaxs = mCameraPosition + mSpawnRange.mMaxs;

			// If Using A Spawn Plane, Increase The Range Box A Bit To Account For Rotation On The Spawn Plane
			//-------------------------------------------------------------------------------------------------
			if (UseSpawnPlane())
			{
				for (int dim=0; dim<3; dim++)
				{
					if (force[dim]>0.01)
					{
						mRange.mMins[dim] -= (mSpawnPlaneDistance/2.0f);
					}
					else if (force[dim]<-0.01)
					{
						mRange.mMaxs[dim] += (mSpawnPlaneDistance/2.0f);
					}
				}
				mSpawnPlaneNorm	= force;
				mSpawnSpeed		= VectorNormalize(mSpawnPlaneNorm.v);
				MakeNormalVectors(mSpawnPlaneNorm.v, mSpawnPlaneRight.v, mSpawnPlaneUp.v);
			}

			// Optimization For Quad Position Calculation
			//--------------------------------------------
			if (mVertexCount==4)
			{
		 		mCameraLeftPlusUp  = (mCameraLeft - mCameraDown);
				mCameraLeftMinusUp = (mCameraLeft + mCameraDown);
			}
			else
			{
				mCameraLeftPlusUp  = (mCameraDown + mCameraLeft);		// should really be called mCamera Left + Down
			}
		}

		// Stop All Additional Processing
		//--------------------------------
		if (mFrozen)
		{
			return;
		}



		// Now Update All Particles
		//--------------------------
		mParticleCountRender = 0;
		for (particleNum=0; particleNum<mParticleCount; particleNum++)
		{
			part			= &(mParticles[particleNum]);

			if (!mPopulated)
			{
				mRange.Pick(part->mPosition);		// First Time Spawn Location
			}

			// Grab The Force And Apply Non Global Wind
			//------------------------------------------
			partForce = force;

			if (numLocalWindZones)
			{
				for (curLocalWindZone=0; curLocalWindZone<numLocalWindZones; curLocalWindZone++)
				{
					if (mLocalWindZones[curLocalWindZone]->mRBounds.In(part->mPosition))
					{
						partForce += mLocalWindZones[curLocalWindZone]->mCurrentVelocity;
					}
				}
			}

			partForce /= part->mMass;


			// Apply The Force
			//-----------------
			part->mVelocity		+= partForce;
			part->mVelocity		*= mFrictionInverse;

			part->mPosition.ScaleAdd(part->mVelocity, mSecondsElapsed);

			partToCamera	= (part->mPosition - mCameraPosition);
			partRendering	= part->mFlags.get_bit(WFXParticle::FLAG_RENDER);
			partOutside		= mOutside.PointOutside(part->mPosition, mWidth, mHeight);
			partInRange		= mRange.In(part->mPosition);
			partInView		= (partOutside && partInRange && (partToCamera.Dot(mCameraForward)>0.0f));

			// Process Respawn
			//-----------------
			if (!partInRange && !partRendering)
			{
				part->mVelocity.Clear();

				// Reselect A Position On The Spawn Plane
				//----------------------------------------
				if (UseSpawnPlane())
				{
					part->mPosition		= mCameraPosition;
					part->mPosition		-= (mSpawnPlaneNorm* mSpawnPlaneDistance);
					part->mPosition		+= (mSpawnPlaneRight*Q_flrand(-mSpawnPlaneSize, mSpawnPlaneSize));
					part->mPosition		+= (mSpawnPlaneUp*   Q_flrand(-mSpawnPlaneSize, mSpawnPlaneSize));
				}

				// Otherwise, Just Wrap Around To The Other End Of The Range
				//-----------------------------------------------------------
				else
				{
					mRange.Wrap(part->mPosition);
				}
				partInRange = true;
			}

			// Process Fade
			//--------------
			{
				// Start A Fade Out
				//------------------
				if		(partRendering && !partInView)
				{
					part->mFlags.clear_bit(WFXParticle::FLAG_FADEIN);
					part->mFlags.set_bit(WFXParticle::FLAG_FADEOUT);
				}

				// Switch From Fade Out To Fade In
				//---------------------------------
				else if (partRendering && partInView && part->mFlags.get_bit(WFXParticle::FLAG_FADEOUT))
				{
					part->mFlags.set_bit(WFXParticle::FLAG_FADEIN);
					part->mFlags.clear_bit(WFXParticle::FLAG_FADEOUT);
				}

				// Start A Fade In
				//-----------------
				else if (!partRendering && partInView)
				{
					partRendering = true;
					part->mAlpha = 0.0f;
					part->mFlags.set_bit(WFXParticle::FLAG_RENDER);
					part->mFlags.set_bit(WFXParticle::FLAG_FADEIN);
					part->mFlags.clear_bit(WFXParticle::FLAG_FADEOUT);
				}

				// Update Fade
				//-------------
				if (partRendering)
				{

					// Update Fade Out
					//-----------------
					if (part->mFlags.get_bit(WFXParticle::FLAG_FADEOUT))
					{
						part->mAlpha -= particleFade;
						if (part->mAlpha<=0.0f)
						{
							part->mAlpha = 0.0f;
							part->mFlags.clear_bit(WFXParticle::FLAG_FADEOUT);
							part->mFlags.clear_bit(WFXParticle::FLAG_FADEIN);
							part->mFlags.clear_bit(WFXParticle::FLAG_RENDER);
							partRendering = false;
						}
					}

					// Update Fade In
					//----------------
					else if (part->mFlags.get_bit(WFXParticle::FLAG_FADEIN))
					{
						part->mAlpha += particleFade;
						if (part->mAlpha>=mColor[3])
						{
							part->mFlags.clear_bit(WFXParticle::FLAG_FADEIN);
							part->mAlpha = mColor[3];
						}
					}
				}
			}

			// Keep Track Of The Number Of Particles To Render
			//-------------------------------------------------
			if (part->mFlags.get_bit(WFXParticle::FLAG_RENDER))
			{
				mParticleCountRender ++;
			}
		}
		mPopulated = true;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Render -
	////////////////////////////////////////////////////////////////////////////////////
	void		Render()
	{
		WFXParticle*	part=0;
		int			particleNum;
		CVec3		partDirection;


		// Set The GL State And Image Binding
		//------------------------------------
		GL_State((mBlendMode==0)?(GLS_ALPHA):(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE));
		GL_Bind(mImage);


		// Enable And Disable Things
		//---------------------------
		qglEnable(GL_TEXTURE_2D);
		qglDisable(GL_CULL_FACE);

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilterMode==0)?(GL_LINEAR):(GL_NEAREST));
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mFilterMode==0)?(GL_LINEAR):(GL_NEAREST));


		// Setup Matrix Mode And Translation
		//-----------------------------------
		qglMatrixMode(GL_MODELVIEW);
		qglPushMatrix();


		// Begin
		//-------
		qglBegin(mGLModeEnum);
		for (particleNum=0; particleNum<mParticleCount; particleNum++)
		{
			part = &(mParticles[particleNum]);
			if (!part->mFlags.get_bit(WFXParticle::FLAG_RENDER))
			{
				continue;
			}

			// If Oriented With Velocity, We Want To Calculate Vertx Offsets Differently For Each Particle
			//---------------------------------------------------------------------------------------------
			if (mOrientWithVelocity)
			{
				partDirection = part->mVelocity;
				VectorNormalize(partDirection.v);
				mCameraDown = partDirection;
				mCameraDown *= (mHeight * -1);
				if (mVertexCount==4)
				{
		 			mCameraLeftPlusUp  = (mCameraLeft - mCameraDown);
					mCameraLeftMinusUp = (mCameraLeft + mCameraDown);
				}
				else
				{
					mCameraLeftPlusUp  = (mCameraDown + mCameraLeft);
				}
			}

			// Blend Mode Zero -> Apply Alpha Just To Alpha Channel
			//------------------------------------------------------
			if (mBlendMode==0)
			{
				qglColor4f(mColor[0], mColor[1], mColor[2], part->mAlpha);
			}

			// Otherwise Apply Alpha To All Channels
			//---------------------------------------
			else
			{
				qglColor4f(mColor[0]*part->mAlpha, mColor[1]*part->mAlpha, mColor[2]*part->mAlpha, mColor[3]*part->mAlpha);
			}


			// Render A Triangle
			//-------------------
			if (mVertexCount==3)
			{
 				qglTexCoord2f(1.0, 0.0);
				qglVertex3f(part->mPosition[0],
							part->mPosition[1],
							part->mPosition[2]);

				qglTexCoord2f(0.0, 1.0);
				qglVertex3f(part->mPosition[0] + mCameraLeft[0],
							part->mPosition[1] + mCameraLeft[1],
							part->mPosition[2] + mCameraLeft[2]);

				qglTexCoord2f(0.0, 0.0);
				qglVertex3f(part->mPosition[0] + mCameraLeftPlusUp[0],
							part->mPosition[1] + mCameraLeftPlusUp[1],
							part->mPosition[2] + mCameraLeftPlusUp[2]);
			}

			// Render A Quad
			//---------------
			else
			{
				// Left bottom.
				qglTexCoord2f( 0.0, 0.0 );
				qglVertex3f(part->mPosition[0] - mCameraLeftMinusUp[0],
							part->mPosition[1] - mCameraLeftMinusUp[1],
							part->mPosition[2] - mCameraLeftMinusUp[2] );

				// Right bottom.
				qglTexCoord2f( 1.0, 0.0 );
				qglVertex3f(part->mPosition[0] - mCameraLeftPlusUp[0],
							part->mPosition[1] - mCameraLeftPlusUp[1],
							part->mPosition[2] - mCameraLeftPlusUp[2] );

				// Right top.
				qglTexCoord2f( 1.0, 1.0 );
				qglVertex3f(part->mPosition[0] + mCameraLeftMinusUp[0],
							part->mPosition[1] + mCameraLeftMinusUp[1],
							part->mPosition[2] + mCameraLeftMinusUp[2] );

				// Left top.
				qglTexCoord2f( 0.0, 1.0 );
				qglVertex3f(part->mPosition[0] + mCameraLeftPlusUp[0],
							part->mPosition[1] + mCameraLeftPlusUp[1],
							part->mPosition[2] + mCameraLeftPlusUp[2] );
			}
		}
		qglEnd();

		qglEnable(GL_CULL_FACE);
		qglPopMatrix();

		mParticlesRendered += mParticleCountRender;
	}
};
ratl::vector_vs<CParticleCloud, MAX_PARTICLE_CLOUDS>	mParticleClouds;



////////////////////////////////////////////////////////////////////////////////////////
// Init World Effects - Will Iterate Over All Particle Clouds, Clear Them Out, And Erase
////////////////////////////////////////////////////////////////////////////////////////
void R_InitWorldEffects(void)
{
	for (int i=0; i<mParticleClouds.size(); i++)
	{
		mParticleClouds[i].Reset();
	}
	mParticleClouds.clear();
	mWindZones.clear();
	mLocalWindZones.clear();
	mOutside.Reset();
	mGlobalWindSpeed = 0.0f;
	mGlobalWindDirection[0]=1.0f;
	mGlobalWindDirection[1]=0.0f;
	mGlobalWindDirection[2]=0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
// Init World Effects - Will Iterate Over All Particle Clouds, Clear Them Out, And Erase
////////////////////////////////////////////////////////////////////////////////////////
void R_ShutdownWorldEffects(void)
{
	R_InitWorldEffects();
}

////////////////////////////////////////////////////////////////////////////////////////
// RB_RenderWorldEffects - If any particle clouds exist, this will update and render them
////////////////////////////////////////////////////////////////////////////////////////
void RB_RenderWorldEffects(void)
{
	if (!tr.world ||
		(tr.refdef.rdflags & RDF_NOWORLDMODEL) ||
		(backEnd.refdef.rdflags & RDF_SKYBOXPORTAL) ||
		!mParticleClouds.size() ||
		ri.CL_IsRunningInGameCinematic())
	{	//  no world rendering or no world or no particle clouds
		return;
	}

	SetViewportAndScissor();
	qglMatrixMode(GL_MODELVIEW);
	qglLoadMatrixf(backEnd.viewParms.world.modelMatrix);


	// Calculate Elapsed Time For Scale Purposes
	//-------------------------------------------
	mMillisecondsElapsed = backEnd.refdef.frametime;
	if (mMillisecondsElapsed<1)
	{
		mMillisecondsElapsed = 1.0f;
	}
	if (mMillisecondsElapsed>1000.0f)
	{
		mMillisecondsElapsed = 1000.0f;
	}
	mSecondsElapsed = (mMillisecondsElapsed / 1000.0f);


	// Make Sure We Are Always Outside Cached
	//----------------------------------------
	if (!mOutside.Initialized())
	{
		mOutside.Cache();
	}
	else
	{
		// Update All Wind Zones
		//-----------------------
		if (!mFrozen)
		{
			mGlobalWindVelocity.Clear();
			for (int wz=0; wz<mWindZones.size(); wz++)
			{
				mWindZones[wz].Update();
				if (mWindZones[wz].mGlobal)
				{
					mGlobalWindVelocity += mWindZones[wz].mCurrentVelocity;
				}
			}
			mGlobalWindDirection	= mGlobalWindVelocity;
			mGlobalWindSpeed		= VectorNormalize(mGlobalWindDirection.v);
		}

		// Update All Particle Clouds
		//----------------------------
		mParticlesRendered = 0;
		for (int i=0; i<mParticleClouds.size(); i++)
		{
			mParticleClouds[i].Update();
			mParticleClouds[i].Render();
		}
		if (false)
		{
			Com_Printf( "Weather: %d Particles Rendered\n", mParticlesRendered);
		}
	}
}


void R_WorldEffect_f(void)
{
	if (ri.Cvar_VariableIntegerValue("helpUsObi"))
	{
		char	temp[2048];
		ri.Cmd_ArgsBuffer(temp, sizeof(temp));
		R_WorldEffectCommand(temp);
	}
}

/*
==================
WE_ParseVector
Imported from MP/Ensiform's fixes --eez
==================
*/

qboolean WE_ParseVector( const char **text, int count, float *v ) {
	char	*token;
	int		i;
	// FIXME: spaces are currently required after parens, should change parseext...
	COM_BeginParseSession();
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		Com_Printf ("^3WARNING: missing parenthesis in weather effect\n" );
		COM_EndParseSession();
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			Com_Printf ("^3WARNING: missing vector element in weather effect\n" );
			COM_EndParseSession();
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	COM_EndParseSession();
	if ( strcmp( token, ")" ) ) {
		Com_Printf ("^3WARNING: missing parenthesis in weather effect\n" );
		return qfalse;
	}
	return qtrue;
}


void R_WorldEffectCommand(const char *command)
{
	if ( !command )
	{
		return;
	}

	const char	*token;//, *origCommand;

	COM_BeginParseSession();

	token = COM_ParseExt(&command, qfalse);

	if ( !token )
	{
		COM_EndParseSession();
		return;
	}


	// Clear - Removes All Particle Clouds And Wind Zones
	//----------------------------------------------------
	if (Q_stricmp(token, "clear") == 0)
	{
		for (int p=0; p<mParticleClouds.size(); p++)
		{
			mParticleClouds[p].Reset();
		}
		mParticleClouds.clear();
		mWindZones.clear();
		mLocalWindZones.clear();
	}

	// Freeze / UnFreeze - Stops All Particle Motion Updates
	//--------------------------------------------------------
	else if (Q_stricmp(token, "freeze") == 0)
	{
		mFrozen = !mFrozen;
	}

	// Add a zone
	//---------------
	else if (Q_stricmp(token, "zone") == 0)
	{
		vec3_t	mins;
		vec3_t	maxs;
		if (WE_ParseVector(&command, 3, mins) && WE_ParseVector(&command, 3, maxs))
		{
			mOutside.AddWeatherZone(mins, maxs);
		}
	}

	// Basic Wind
	//------------
	else if (Q_stricmp(token, "wind") == 0)
	{
		if (mWindZones.full())
		{
			COM_EndParseSession();
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
	}

	// Constant Wind
	//---------------
	else if (Q_stricmp(token, "constantwind") == 0)
	{
		if (mWindZones.full())
		{
			COM_EndParseSession();
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
		if (!WE_ParseVector(&command, 3, nWind.mCurrentVelocity.v))
		{
			nWind.mCurrentVelocity.Clear();
			nWind.mCurrentVelocity[1] = 800.0f;
		}
		nWind.mTargetVelocityTimeRemaining = -1;
	}

	// Gusting Wind
	//--------------
	else if (Q_stricmp(token, "gustingwind") == 0)
	{
		if (mWindZones.full())
		{
			COM_EndParseSession();
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
		nWind.mRVelocity.mMins				= -3000.0f;
		nWind.mRVelocity.mMins[2]			= -100.0f;
		nWind.mRVelocity.mMaxs				=  3000.0f;
		nWind.mRVelocity.mMaxs[2]			=  100.0f;

		nWind.mMaxDeltaVelocityPerUpdate	=  10.0f;

		nWind.mRDuration.mMin				=  1000;
		nWind.mRDuration.mMax				=  3000;

		nWind.mChanceOfDeadTime				=  0.5f;
		nWind.mRDeadTime.mMin				=  2000;
		nWind.mRDeadTime.mMax				=  4000;
	}

	// Local Wind Zone
	//-----------------
	else if (Q_stricmp(token, "windzone") == 0)
	{
		if (mWindZones.full())
		{
			COM_EndParseSession();
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();

		nWind.mGlobal = false;

		// Read Mins
		if (!WE_ParseVector(&command, 3, nWind.mRBounds.mMins.v))
		{
			assert("Wind Zone: Unable To Parse Mins Vector!"==0);
			mWindZones.pop_back();
			COM_EndParseSession();
			return;
		}

		// Read Maxs
		if (!WE_ParseVector(&command, 3, nWind.mRBounds.mMaxs.v))
		{
			assert("Wind Zone: Unable To Parse Maxs Vector!"==0);
			mWindZones.pop_back();
			COM_EndParseSession();
			return;
		}

		// Read Velocity
		if (!WE_ParseVector(&command, 3, nWind.mCurrentVelocity.v))
		{
			nWind.mCurrentVelocity.Clear();
			nWind.mCurrentVelocity[1] = 800.0f;
		}
		nWind.mTargetVelocityTimeRemaining = -1;

		mLocalWindZones.push_back(&nWind);
	}


	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "lightrain") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(500, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 2000.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 100.0f;
		nCloud.mColor		= 0.5f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "rain") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 2000.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 100.0f;
		nCloud.mColor		= 0.5f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "acidrain") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 2.0f;
		nCloud.mGravity		= 2000.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 100.0f;

		nCloud.mColor[0]	= 0.34f;
		nCloud.mColor[1]	= 0.70f;
		nCloud.mColor[2]	= 0.34f;
		nCloud.mColor[3]	= 0.70f;

		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;

		mOutside.mOutsidePain = 0.1f;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "heavyrain") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 2800.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 15.0f;
		nCloud.mColor		= 0.5f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Snow Storm
	//---------------------
	else if (Q_stricmp(token, "snow") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/effects/snowflake1.bmp");
		nCloud.mBlendMode			= 1;
		nCloud.mRotationChangeNext	= 0;
		nCloud.mColor		= 0.75f;
		nCloud.mWaterParticles = true;
	}

	// Create A Some stuff
	//---------------------
	else if (Q_stricmp(token, "spacedust") == 0)
	{
		int count;
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		token = COM_ParseExt(&command, qfalse);
		count = atoi(token);

		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(count, "gfx/effects/snowpuff1.tga");
		nCloud.mHeight		= 1.2f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 0.0f;
		nCloud.mBlendMode			= 1;
		nCloud.mRotationChangeNext	= 0;
		nCloud.mColor		= 0.75f;
		nCloud.mWaterParticles = true;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[0]	= -1500.0f;
		nCloud.mSpawnRange.mMins[1]	= -1500.0f;
		nCloud.mSpawnRange.mMins[2]	= -1500.0f;
		nCloud.mSpawnRange.mMaxs[0]	= 1500.0f;
		nCloud.mSpawnRange.mMaxs[1]	= 1500.0f;
		nCloud.mSpawnRange.mMaxs[2]	= 1500.0f;
	}

	// Create A Sand Storm
	//---------------------
	else if (Q_stricmp(token, "sand") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(400, "gfx/effects/alpha_smoke2b.tga");

		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 70;
		nCloud.mHeight		= 70;
		nCloud.mColor[0]	= 0.9f;
		nCloud.mColor[1]	= 0.6f;
		nCloud.mColor[2]	= 0.0f;
		nCloud.mColor[3]	= 0.5f;
		nCloud.mFade		= 5.0f;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	= 150;

		nCloud.mRotationChangeNext	= 0;
	}

	// Create Blowing Clouds Of Fog
	//------------------------------
	else if (Q_stricmp(token, "fog") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(60, "gfx/effects/alpha_smoke2b.tga");
		nCloud.mBlendMode	= 1;
		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 70;
		nCloud.mHeight		= 70;
		nCloud.mColor		= 0.2f;
		nCloud.mFade		= 5.0f;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	= 150;

		nCloud.mRotationChangeNext	= 0;
	}

	// Create Heavy Rain Particle Cloud
	//-----------------------------------
	else if (Q_stricmp(token, "heavyrainfog") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
 		nCloud.Initialize(70, "gfx/effects/alpha_smoke2b.tga");
		nCloud.mBlendMode	= 1;
		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 100;
		nCloud.mHeight		= 100;
		nCloud.mColor		= 0.3f;
		nCloud.mFade		= 1.0f;
		nCloud.mMass.mMax	= 10.0f;
		nCloud.mMass.mMin	= 5.0f;

		nCloud.mSpawnRange.mMins	= -(nCloud.mSpawnPlaneDistance*1.25f);
		nCloud.mSpawnRange.mMaxs	=  (nCloud.mSpawnPlaneDistance*1.25f);
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	=  150;

		nCloud.mRotationChangeNext	= 0;
	}

	// Create Blowing Clouds Of Fog
	//------------------------------
	else if (Q_stricmp(token, "light_fog") == 0)
	{
		if (mParticleClouds.full())
		{
			COM_EndParseSession();
			return;
		}
		CParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(40, "gfx/effects/alpha_smoke2b.tga");
		nCloud.mBlendMode	= 1;
		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 100;
		nCloud.mHeight		= 100;
		nCloud.mColor[0]	= 0.19f;
		nCloud.mColor[1]	= 0.6f;
		nCloud.mColor[2]	= 0.7f;
		nCloud.mColor[3]	= 0.12f;
		nCloud.mFade		= 0.10f;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	= 150;

		nCloud.mRotationChangeNext	= 0;
	}
	else if (Q_stricmp(token, "outsideshake") == 0)
	{
		mOutside.mOutsideShake = !mOutside.mOutsideShake;
	}
	else if (Q_stricmp(token, "outsidepain") == 0)
	{
		mOutside.mOutsidePain = !mOutside.mOutsidePain;
	}
	else
	{
		Com_Printf( "Weather Effect: Please enter a valid command.\n" );
		Com_Printf( "	clear\n" );
		Com_Printf( "	freeze\n" );
		Com_Printf( "	zone (mins) (maxs)\n" );
		Com_Printf( "	wind\n" );
		Com_Printf( "	constantwind (velocity)\n" );
		Com_Printf( "	gustingwind\n" );
		Com_Printf( "	windzone (mins) (maxs) (velocity)\n" );
		Com_Printf( "	lightrain\n" );
		Com_Printf( "	rain\n" );
		Com_Printf( "	acidrain\n" );
		Com_Printf( "	heavyrain\n" );
		Com_Printf( "	snow\n" );
		Com_Printf( "	spacedust\n" );
		Com_Printf( "	sand\n" );
		Com_Printf( "	fog\n" );
		Com_Printf( "	heavyrainfog\n" );
		Com_Printf( "	light_fog\n" );
		Com_Printf( "	outsideshake\n" );
		Com_Printf( "	outsidepain\n" );
	}
	COM_EndParseSession();
}




float R_GetChanceOfSaberFizz()
{
 	float	chance = 0.0f;
	int		numWater = 0;
	for (int i=0; i<mParticleClouds.size(); i++)
	{
		if (mParticleClouds[i].mWaterParticles)
		{
			chance += (mParticleClouds[i].mGravity/20000.0f);
			numWater ++;
		}
	}
	if (numWater)
	{
		return (chance / numWater);
	}
	return 0.0f;
}

bool R_IsRaining()
{
	return !mParticleClouds.empty();
}

bool R_IsPuffing()
{
	return false;
}




