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

#if !defined(FX_UTIL_H_INC)
	#include "FxUtil.h"
#endif


#include "../../code/qcommon/sstring.h"
typedef sstring_t fxString_t;

#include "../game/genericparser2.h"
#include "qcommon/safe/string.h"

#include <algorithm>

#ifndef FX_SCHEDULER_H_INC
#define FX_SCHEDULER_H_INC

#define FX_FILE_PATH	"effects"

#define FX_MAX_TRACE_DIST			WORLD_SIZE
#define FX_MAX_EFFECTS				150		// how many effects the system can store
#define FX_MAX_EFFECT_COMPONENTS	24		// how many primitives an effect can hold, this should be plenty
#define FX_MAX_PRIM_NAME			32
	
//-----------------------------------------------
// These are spawn flags for primitiveTemplates
//-----------------------------------------------

#define FX_ORG_ON_SPHERE		0x00001	// Pretty dang expensive, calculates a point on a sphere/ellipsoid
#define FX_AXIS_FROM_SPHERE		0x00002	// Can be used in conjunction with org_on_sphere to cause particles to move out 
										//	from the center of the sphere
#define FX_ORG_ON_CYLINDER		0x00004	// calculate point on cylinder/disk

#define FX_ORG2_FROM_TRACE		0x00010
#define FX_TRACE_IMPACT_FX		0x00020	// if trace impacts, we should play one of the specified impact fx files
#define FX_ORG2_IS_OFFSET		0x00040	// template specified org2 should be the offset from a trace endpos or 
										//	passed in org2. You might use this to lend a random flair to the endpos.
										//	Note: this is done pre-trace, so you may have to specify large numbers for this

#define FX_CHEAP_ORG_CALC		0x00100	// Origin is calculated relative to passed in axis unless this is on.
#define FX_CHEAP_ORG2_CALC		0x00200	// Origin2 is calculated relative to passed in axis unless this is on.
#define FX_VEL_IS_ABSOLUTE		0x00400	// Velocity isn't relative to passed in axis with this flag on.
#define FX_ACCEL_IS_ABSOLUTE	0x00800	// Acceleration isn't relative to passed in axis with this flag on.

#define FX_RAND_ROT_AROUND_FWD	0x01000	// Randomly rotates up and right around forward vector
#define FX_EVEN_DISTRIBUTION	0x02000	// When you have a delay, it normally picks a random time to play.  When
										// this flag is on, it generates an even time distribution
#define FX_RGB_COMPONENT_INTERP	0x04000	// Picks a color on the line defined by RGB min & max, default is to pick color in cube defined by min & max

#define FX_AFFECTED_BY_WIND		0x10000 // we take into account our wind vector when we spawn in

#define FX_SND_LESS_ATTENUATION	0x20000	// attenuate sounds less

//-----------------------------------------------------------------
//
// CMediaHandles
//
// Primitive templates might want to use a list of sounds, shaders 
//	or models to get a bit more variation in their effects.
//
//-----------------------------------------------------------------
class CMediaHandles
{
private:

	std::vector<int>	mMediaList;

public:

	void	AddHandle( int item )	{ mMediaList.push_back( item );	}
	int		GetHandle()				{ if (mMediaList.size()==0) {return 0;}
										else {return mMediaList[irand(0,(int)mMediaList.size()-1)];} }

	void operator=(const CMediaHandles &that );
};


//-----------------------------------------------------------------
//
// CFxRange
//
// Primitive templates typically use this class to define each of
//	its members.  This is done to make it easier to create effects 
//	with a desired range of characteristics.
//
//-----------------------------------------------------------------
class CFxRange
{
private:

	float	mMin;
	float	mMax;

public:

	CFxRange()										{mMin=0; mMax=0;}

	inline void		SetRange(float min,float max)	{mMin=min; mMax=max;}
	inline void		SetMin(float min)				{mMin=min;}
	inline void		SetMax(float max)				{mMax=max;}

	inline float	GetMax() const					{return mMax;}
	inline float	GetMin() const					{return mMin;}

	inline float	GetVal(float percent) const		{return (mMin + (mMax - mMin) * percent);}
	inline float	GetVal() const					{if(mMin == mMax){return mMin;}
														return flrand(mMin, mMax);}
	inline int		GetRoundedVal() const			{if(mMin == mMax){return mMin;}
														return (int)(flrand(mMin, mMax) + 0.5f);}

	inline void		ForceRange(float min,float max)	{if(mMin < min){mMin=min;} if(mMin > max){mMin=max;}
														if(mMax < min){mMax=min;} if(mMax > max){mMax=max;}}
	inline void		Sort()							{if(mMin > mMax){float temp = mMin; mMin=mMax;mMax=temp;}}
	void operator=(const CFxRange &that)			{mMin=that.mMin; mMax=that.mMax;}

	bool operator==(const CFxRange &rhs) const		{ return ((mMin == rhs.mMin) &&
															  (mMax == rhs.mMax)); }
};


//----------------------------
// Supported primitive types
//----------------------------

enum EPrimType
{
	None = 0,
	Particle,		// sprite
	Line,
	Tail,			// comet-like tail thing
	Cylinder,
	Emitter,		// emits effects as it moves, can also attach a chunk
	Sound,
	Decal,			// projected onto architecture
	OrientedParticle,
	Electricity,
	FxRunner,
	Light,
	CameraShake,
	ScreenFlash
};


//-----------------------------------------------------------------
//
// CPrimitiveTemplate
//
// The primitive template is used to spawn 1 or more fx primitives 
//	with the range of characteristics defined by the template.
//
// As such, I just made this one huge shared class knowing that 
//	there won't be many of them in memory at once, 	and we won't 
//	be dynamically creating and deleting them mid-game.  Also,
//	note that not every primitive type will use all of these fields.
//
//-----------------------------------------------------------------
class CPrimitiveTemplate
{

public:

	// These kinds of things should not even be allowed to be accessed publicly
	bool			mCopy;
	int				mRefCount;		// For a copy of a primitive...when we figure out how many items we want to spawn, 
									//	we'll store that here and then decrement us for each we actually spawn.  When we 
									//	hit zero, we are no longer used and so we can just free ourselves

	char			mName[FX_MAX_PRIM_NAME];

	EPrimType		mType;

	CFxRange		mSpawnDelay;
	CFxRange		mSpawnCount;
	CFxRange		mLife;
	int				mCullRange;

	CMediaHandles	mMediaHandles;
	CMediaHandles	mImpactFxHandles;
	CMediaHandles	mDeathFxHandles;
	CMediaHandles	mEmitterFxHandles;
	CMediaHandles	mPlayFxHandles;

	int				mFlags;			// These need to get passed on to the primitive
	int				mSpawnFlags;	// These are only used to control spawning, but never get passed to prims.

	vec3_t			mMin;
	vec3_t			mMax;

	CFxRange		mOrigin1X;
	CFxRange		mOrigin1Y;
	CFxRange		mOrigin1Z;

	CFxRange		mOrigin2X;
	CFxRange		mOrigin2Y;
	CFxRange		mOrigin2Z;

	CFxRange		mRadius;		// spawn on sphere/ellipse/disk stuff.
	CFxRange		mHeight;

	CFxRange		mRotation;
	CFxRange		mRotationDelta;

	CFxRange		mAngle1;
	CFxRange		mAngle2;
	CFxRange		mAngle3;

	CFxRange		mAngle1Delta;
	CFxRange		mAngle2Delta;
	CFxRange		mAngle3Delta;

	CFxRange		mVelX;
	CFxRange		mVelY;
	CFxRange		mVelZ;

	CFxRange		mAccelX;
	CFxRange		mAccelY;
	CFxRange		mAccelZ;

	CFxRange		mGravity;

	CFxRange		mDensity;
	CFxRange		mVariance;

	CFxRange		mRedStart;
	CFxRange		mGreenStart;
	CFxRange		mBlueStart;

	CFxRange		mRedEnd;
	CFxRange		mGreenEnd;
	CFxRange		mBlueEnd;

	CFxRange		mRGBParm;

	CFxRange		mAlphaStart;
	CFxRange		mAlphaEnd;
	CFxRange		mAlphaParm;

	CFxRange		mSizeStart;
	CFxRange		mSizeEnd;
	CFxRange		mSizeParm;

	CFxRange		mSize2Start;
	CFxRange		mSize2End;
	CFxRange		mSize2Parm;

	CFxRange		mLengthStart;
	CFxRange		mLengthEnd;
	CFxRange		mLengthParm;

	CFxRange		mTexCoordS;
	CFxRange		mTexCoordT;

	CFxRange		mElasticity;

private:

	// Lower level parsing utilities
	bool ParseVector( const gsl::cstring_view& val, vec3_t min, vec3_t max );
	bool ParseFloat( const gsl::cstring_view& val, float& min, float& max );
	bool ParseGroupFlags( const gsl::cstring_view& val, int& flags );

	// Base key processing
	// Note that these all have their own parse functions in case it becomes important to do certain kinds
	//	of validation specific to that type.
	bool ParseMin( const gsl::cstring_view& val );
	bool ParseMax( const gsl::cstring_view& val );
	bool ParseDelay( const gsl::cstring_view& val );
	bool ParseCount( const gsl::cstring_view& val );
	bool ParseLife( const gsl::cstring_view& val );
	bool ParseElasticity( const gsl::cstring_view& val );
	bool ParseFlags( const gsl::cstring_view& val );
	bool ParseSpawnFlags( const gsl::cstring_view& val );

	bool ParseOrigin1( const gsl::cstring_view& val );
	bool ParseOrigin2( const gsl::cstring_view& val );
	bool ParseRadius( const gsl::cstring_view& val );
	bool ParseHeight( const gsl::cstring_view& val );
	bool ParseRotation( const gsl::cstring_view& val );
	bool ParseRotationDelta( const gsl::cstring_view& val );
	bool ParseAngle( const gsl::cstring_view& val );
	bool ParseAngleDelta( const gsl::cstring_view& val );
	bool ParseVelocity( const gsl::cstring_view& val );
	bool ParseAcceleration( const gsl::cstring_view& val );
	bool ParseGravity( const gsl::cstring_view& val );
	bool ParseDensity( const gsl::cstring_view& val );
	bool ParseVariance( const gsl::cstring_view& val );

	/// Case insensitive map from cstring_view to Value
	template< typename Value >
	using StringViewIMap = std::map< gsl::cstring_view, Value, Q::CStringViewILess >;
	using ParseMethod = bool ( CPrimitiveTemplate::* )( const gsl::cstring_view& );
	// Group type processing
	bool ParseGroup( const CGPGroup& grp, const StringViewIMap< ParseMethod >& parseMethods, gsl::czstring name );
	bool ParseRGB( const CGPGroup& grp );
	bool ParseAlpha( const CGPGroup& grp );
	bool ParseSize( const CGPGroup& grp );
	bool ParseSize2( const CGPGroup& grp );
	bool ParseLength( const CGPGroup& grp );

	bool ParseModels( const CGPProperty& grp );
	bool ParseShaders( const CGPProperty& grp );
	bool ParseSounds( const CGPProperty& grp );

	bool ParseImpactFxStrings( const CGPProperty& grp );
	bool ParseDeathFxStrings( const CGPProperty& grp );
	bool ParseEmitterFxStrings( const CGPProperty& grp );
	bool ParsePlayFxStrings( const CGPProperty& grp );

	// Group keys
	bool ParseRGBStart( const gsl::cstring_view& val );
	bool ParseRGBEnd( const gsl::cstring_view& val );
	bool ParseRGBParm( const gsl::cstring_view& val );
	bool ParseRGBFlags( const gsl::cstring_view& val );

	bool ParseAlphaStart( const gsl::cstring_view& val );
	bool ParseAlphaEnd( const gsl::cstring_view& val );
	bool ParseAlphaParm( const gsl::cstring_view& val );
	bool ParseAlphaFlags( const gsl::cstring_view& val );

	bool ParseSizeStart( const gsl::cstring_view& val );
	bool ParseSizeEnd( const gsl::cstring_view& val );
	bool ParseSizeParm( const gsl::cstring_view& val );
	bool ParseSizeFlags( const gsl::cstring_view& val );

	bool ParseSize2Start( const gsl::cstring_view& val );
	bool ParseSize2End( const gsl::cstring_view& val );
	bool ParseSize2Parm( const gsl::cstring_view& val );
	bool ParseSize2Flags( const gsl::cstring_view& val );

	bool ParseLengthStart( const gsl::cstring_view& val );
	bool ParseLengthEnd( const gsl::cstring_view& val );
	bool ParseLengthParm( const gsl::cstring_view& val );
	bool ParseLengthFlags( const gsl::cstring_view& val );


public:

	CPrimitiveTemplate();
	CPrimitiveTemplate( const CPrimitiveTemplate& rhs );
	~CPrimitiveTemplate()	{};

	bool ParsePrimitive( const CGPGroup& grp );

	void operator=(const CPrimitiveTemplate &that);
};

// forward declaration
struct SEffectTemplate;

// Effects are built of one or more primitives
struct SEffectTemplate
{
	bool	mInUse;
	bool	mCopy;
	char	mEffectName[MAX_QPATH];					// is this extraneous??
	int		mPrimitiveCount;
	CPrimitiveTemplate	*mPrimitives[FX_MAX_EFFECT_COMPONENTS];

	bool operator == (const char * name) const
	{
		return !Q_stricmp( mEffectName, name );
	}
	void operator=(const SEffectTemplate &that);
};

template<typename T, int N>
class PoolAllocator
{
public:
	PoolAllocator()
		: pool (new T[N])
		, freeAndAllocated (new int[N])
		, numFree (N)
		, highWatermark (0)
	{
		for ( int i = 0; i < N; i++ )
		{
			freeAndAllocated[i] = i;
		}
	}

	T *Alloc()
	{
		if ( numFree == 0 )
		{
			return NULL;
		}

		T *ptr = new (&pool[freeAndAllocated[0]]) T;

		std::rotate (freeAndAllocated, freeAndAllocated + 1, freeAndAllocated + N);
		numFree--;

		highWatermark = Q_max(highWatermark, N - numFree);

		return ptr;
	}

	void TransferTo ( PoolAllocator<T, N>& allocator )
	{
		allocator.freeAndAllocated = freeAndAllocated;
		allocator.highWatermark = highWatermark;
		allocator.numFree = numFree;
		allocator.pool = pool;

		highWatermark = 0;
		numFree = N;
		freeAndAllocated = NULL;
		pool = NULL;
	}

	bool OwnsPtr ( const T *ptr ) const
	{
		return ptr >= pool && ptr < (pool + N);
	}

	void Free ( T *ptr )
	{
		for ( int i = numFree; i < N; i++ )
		{
			T *p = &pool[freeAndAllocated[i]];

			if ( p == ptr )
			{
				if ( i > numFree )
				{
					std::rotate (freeAndAllocated + numFree, freeAndAllocated + i, freeAndAllocated + i + 1);
				}

				p->~T();
				numFree++;

				break;
			}
		}
	}

	int GetHighWatermark() const { return highWatermark; }

	~PoolAllocator()
	{
		for ( int i = numFree; i < N; i++ )
		{
			T *p = &pool[freeAndAllocated[i]];

			p->~T();
		}

		delete [] freeAndAllocated;
		delete [] pool;
	}

private:
	PoolAllocator ( const PoolAllocator<T, N>& );
	PoolAllocator& operator = ( const PoolAllocator<T, N>& );

	T *pool;

	// The first 'numFree' elements are the indexes of the free slots.
	// The remaining elements are the indexes of the allocated slots.
	int *freeAndAllocated;
	int numFree;

	int highWatermark;
};

template<typename T, int N>
class PagedPoolAllocator
{
	public:
		PagedPoolAllocator ()
			: numPages (1)
			, pages (new PoolAllocator<T, N>[1]())
		{
		}

		T *Alloc ()
		{
			T *ptr = NULL;
			for ( int i = 0; i < numPages && ptr == NULL; i++ )
			{
				ptr = pages[i].Alloc ();
			}

			if ( ptr == NULL )
			{
				PoolAllocator<T, N> *newPages = new PoolAllocator<T, N>[numPages + 1] ();
				for ( int i = 0; i < numPages; i++ )
				{
					pages[i].TransferTo (newPages[i]);
				}

				delete[] pages;
				pages = newPages;

				ptr = pages[numPages].Alloc ();
				if ( ptr == NULL )
				{
					return NULL;
				}

				numPages++;
			}

			return ptr;
		}

		void Free ( T *ptr )
		{
			for ( int i = 0; i < numPages; i++ )
			{
				if ( pages[i].OwnsPtr (ptr) )
				{
					pages[i].Free (ptr);
					break;
				}
			}
		}

		int GetHighWatermark () const
		{
			int total = 0;
			for ( int i = 0; i < numPages; i++ )
			{
				total += pages[i].GetHighWatermark ();
			}

			return total;
		}

		~PagedPoolAllocator ()
		{
			delete[] pages;
		}

	private:
		int numPages;
		PoolAllocator<T, N> *pages;
};

//-----------------------------------------------------------------
//
// CFxScheduler
//
// The scheduler not only handles requests to play an effect, it 
//	tracks the request throughout its life if necessary, creating 
//	any of the delayed components as needed.
//
//-----------------------------------------------------------------
class CFxScheduler
{
private:

	// We hold a scheduled effect here
	struct SScheduledEffect
	{
		CPrimitiveTemplate	*mpTemplate;	// primitive template
		int		mStartTime;
		int		mModelNum;		// uset to determine which ghoul2 model we want to bolt this effect to
		int		mBoltNum;		// used to determine which bolt on the ghoul2 model we should be attaching this effect to
		int		mEntNum;		// used to determine which entity this ghoul model is attached to.
		int		mClientID;		// FIXME: redundant??
		vec3_t	mOrigin;
		vec3_t	mAxis[3];
	};

	// this makes looking up the index based on the string name much easier
	typedef std::map<fxString_t, int>			TEffectID;

	typedef std::list<SScheduledEffect*>			TScheduledEffect;

	// Effects
	SEffectTemplate		mEffectTemplates[FX_MAX_EFFECTS];
	TEffectID			mEffectIDs;								// if you only have the unique effect name, you'll have to use this to get the ID.

	// List of scheduled effects that will need to be created at the correct time.
	TScheduledEffect	mFxSchedule;

	PagedPoolAllocator<SScheduledEffect, 1024> mScheduledEffectsPool;

	// Private function prototypes
	SEffectTemplate *GetNewEffectTemplate( int *id, const char *file );

	void	AddPrimitiveToEffect( SEffectTemplate *fx, CPrimitiveTemplate *prim );
	int		ParseEffect( const char *file, const CGPGroup& base );

	void	CreateEffect( CPrimitiveTemplate *fx, const vec3_t origin, vec3_t axis[3], int lateTime );
	void	CreateEffect( CPrimitiveTemplate *fx, int clientID, int lateTime );

public:

	CFxScheduler();
	
	int		RegisterEffect( const char *file, bool bHasCorrectPath = false );	// handles pre-caching


	// Nasty overloaded madness
	void	PlayEffect( int id, vec3_t org );							// uses a default up axis
	void	PlayEffect( int id, vec3_t org, vec3_t fwd );				// builds arbitrary perp. right vector, does a cross product to define up
	void	PlayEffect( int id, vec3_t origin, vec3_t axis[3], const int boltInfo=-1, const int entNum=-1 );
	void	PlayEffect( const char *file, vec3_t org );					// uses a default up axis
	void	PlayEffect( const char *file, vec3_t org, vec3_t fwd );		// builds arbitrary perp. right vector, does a cross product to define up
	void	PlayEffect( const char *file, vec3_t origin, 
				vec3_t axis[3], const int boltInfo, const int entNum );

	void	PlayEffect( const char *file, int clientID );

	void	AddScheduledEffects( void );								// call once per CGame frame

	int		NumScheduledFx()	{ return (int)mFxSchedule.size();	}
	void	Clean(bool bRemoveTemplates = true, int idToPreserve = 0);	// clean out the system

	// FX Override functions
	SEffectTemplate		*GetEffectCopy( int fxHandle, int *newHandle );
	SEffectTemplate		*GetEffectCopy( const char *file, int *newHandle );

	CPrimitiveTemplate	*GetPrimitiveCopy( SEffectTemplate *effectCopy, const char *componentName );
};

//-------------------
// The one and only
//-------------------
extern CFxScheduler theFxScheduler;


#endif // FX_SCHEDULER_H_INC
