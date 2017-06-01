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

#include "FxUtil.h"
#include "qcommon/GenericParser2.h"

#include <algorithm>
#include <vector>
#include <map>
#include <list>
#include <string>

#define FX_FILE_PATH	"effects"

#define FX_MAX_TRACE_DIST		16384	// SOF2 uses a larger scale
#define FX_MAX_EFFECTS				256		// how many effects the system can store
#define FX_MAX_2DEFFECTS			64		// how many 2d effects the system can store
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

#define FX_AFFECTED_BY_WIND		0x10000	// this effect primitive needs to query wind

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

	CMediaHandles &operator=(const CMediaHandles &that );
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

	CFxRange(void) 									{ mMin = 0.0f; mMax = 0.0f; }

	inline void		SetRange(float min,float max)	{ mMin = min; mMax = max; }

	inline float	GetMax(void) const				{ return mMax; }
	inline float	GetMin(void) const				{ return mMin; }
	inline float	GetVal(float fraction) const	{ if(mMin != mMax) { return mMin + fraction * (mMax - mMin); } else { return mMin; } }
	inline float	GetVal(void) const	 			{ if(mMin != mMax) { return flrand(mMin,mMax); } else { return mMin; } }

	inline int		GetRoundedVal() const			{if(mMin == mMax){return (int)mMin;}
														return (int)(flrand(mMin, mMax) + 0.5f);}

	bool operator==(const CFxRange &rhs) const		{ return ((mMin == rhs.mMin) &&	(mMax == rhs.mMax)); }
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

	EMatImpactEffect	mMatImpactFX;

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
	CFxRange		mWindModifier;

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

	int				mSoundRadius;
	int				mSoundVolume;

	// Lower level parsing utilities
	bool ParseVector( const char *val, vec3_t min, vec3_t max );
	bool ParseFloat( const char *val, float *min, float *max );
	bool ParseGroupFlags( const char *val, int *flags );

	// Base key processing
	// Note that these all have their own parse functions in case it becomes important to do certain kinds
	//	of validation specific to that type.
	bool ParseMin( const char *val );
	bool ParseMax( const char *val );
	bool ParseDelay( const char *val );
	bool ParseCount( const char *val );
	bool ParseLife( const char *val );
	bool ParseElasticity( const char *val );
	bool ParseFlags( const char *val );
	bool ParseSpawnFlags( const char *val );

	bool ParseOrigin1( const char *val );
	bool ParseOrigin2( const char *val );
	bool ParseRadius( const char *val );
	bool ParseHeight( const char *val );
	bool ParseWindModifier( const char *val );
	bool ParseRotation( const char *val );
	bool ParseRotationDelta( const char *val );
	bool ParseAngle( const char *val );
	bool ParseAngleDelta( const char *val );
	bool ParseVelocity( const char *val );
	bool ParseAcceleration( const char *val );
	bool ParseGravity( const char *val );
	bool ParseDensity( const char *val );
	bool ParseVariance( const char *val );

	// Group type processing
	bool ParseRGB( CGPGroup *grp );
	bool ParseAlpha( CGPGroup *grp );
	bool ParseSize( CGPGroup *grp );
	bool ParseSize2( CGPGroup *grp );
	bool ParseLength( CGPGroup *grp );

	bool ParseModels( CGPValue *grp );
	bool ParseShaders( CGPValue *grp );
	bool ParseSounds( CGPValue *grp );

	bool ParseImpactFxStrings( CGPValue *grp );
	bool ParseDeathFxStrings( CGPValue *grp );
	bool ParseEmitterFxStrings( CGPValue *grp );
	bool ParsePlayFxStrings( CGPValue *grp );

	// Group keys
	bool ParseRGBStart( const char *val );
	bool ParseRGBEnd( const char *val );
	bool ParseRGBParm( const char *val );
	bool ParseRGBFlags( const char *val );

	bool ParseAlphaStart( const char *val );
	bool ParseAlphaEnd( const char *val );
	bool ParseAlphaParm( const char *val );
	bool ParseAlphaFlags( const char *val );

	bool ParseSizeStart( const char *val );
	bool ParseSizeEnd( const char *val );
	bool ParseSizeParm( const char *val );
	bool ParseSizeFlags( const char *val );

	bool ParseSize2Start( const char *val );
	bool ParseSize2End( const char *val );
	bool ParseSize2Parm( const char *val );
	bool ParseSize2Flags( const char *val );

	bool ParseLengthStart( const char *val );
	bool ParseLengthEnd( const char *val );
	bool ParseLengthParm( const char *val );
	bool ParseLengthFlags( const char *val );

	bool ParseMaterialImpact(const char *val);

public:

	CPrimitiveTemplate();
	~CPrimitiveTemplate()	{};

	bool ParsePrimitive( CGPGroup *grp );

	CPrimitiveTemplate &operator=(const CPrimitiveTemplate &that);
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
	int		mRepeatDelay;
	CPrimitiveTemplate	*mPrimitives[FX_MAX_EFFECT_COMPONENTS];

	bool operator == (const char * name) const
	{
		return !Q_stricmp( mEffectName, name );
	}
	SEffectTemplate &operator=(const SEffectTemplate &that);
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
		char	mModelNum;		// uset to determine which ghoul2 model we want to bolt this effect to
		char	mBoltNum;		// used to determine which bolt on the ghoul2 model we should be attaching this effect to
		short	mEntNum;		// used to determine which entity this ghoul model is attached to.
		bool	mPortalEffect;	// rww - render this before skyportals, and not in the normal world view.
		bool	mIsRelative;	// bolt this puppy on keep it updated
		CGhoul2Info_v *ghoul2;
		vec3_t	mOrigin;
		matrix3_t	mAxis;
	};

/* Looped Effects get stored and reschedule at mRepeatRate */
	#define MAX_LOOPED_FX 32
	// We hold a looped effect here
	struct SLoopedEffect
	{
		int		mId;			// effect id
		int		mBoltInfo;		// used to determine which bolt on the ghoul2 model we should be attaching this effect to
		CGhoul2Info_v *mGhoul2;
		int		mNextTime;		//time to render again
		int		mLoopStopTime;	//time to die
		bool	mPortalEffect;	// rww - render this before skyportals, and not in the normal world view.
		bool	mIsRelative;	// bolt this puppy on keep it updated
	};

	SLoopedEffect	mLoopedEffectArray[MAX_LOOPED_FX];

	int		ScheduleLoopedEffect( int id, int boltInfo, CGhoul2Info_v *ghoul2, bool isPortal, int iLoopTime, bool isRelative );
	void	AddLoopedEffects( );


	class CScheduled2DEffect
	{
	public:
		CScheduled2DEffect():
		mScreenX(0),
		mScreenY(0),
		mWidth(0),
		mHeight(0),
		mShaderHandle(0)
		{mColor[0]=mColor[1]=mColor[2]=mColor[3]=1.0f;}

	public:
		float		mScreenX;
		float		mScreenY;
		float		mWidth;
		float		mHeight;
		vec4_t		mColor;		// bytes A, G, B, R -- see class paletteRGBA_c
		qhandle_t	mShaderHandle;
	};

	// this makes looking up the index based on the string name much easier
	typedef std::map<std::string, int>				TEffectID;

	typedef std::list<SScheduledEffect*>			TScheduledEffect;

	// Effects
	SEffectTemplate		mEffectTemplates[FX_MAX_EFFECTS];
	TEffectID			mEffectIDs;								// if you only have the unique effect name, you'll have to use this to get the ID.

	// 2D effects
	CScheduled2DEffect	m2DEffects[FX_MAX_2DEFFECTS];
	int					mNextFree2DEffect;

	// List of scheduled effects that will need to be created at the correct time.
	TScheduledEffect	mFxSchedule;

	PagedPoolAllocator<SScheduledEffect, 1024> mScheduledEffectsPool;

	// Private function prototypes
	SEffectTemplate *GetNewEffectTemplate( int *id, const char *file );

	void	AddPrimitiveToEffect( SEffectTemplate *fx, CPrimitiveTemplate *prim );
	int		ParseEffect( const char *file, CGPGroup *base );

	void	CreateEffect( CPrimitiveTemplate *fx, const vec3_t origin, matrix3_t axis, int lateTime, int fxParm = -1,  CGhoul2Info_v *ghoul2 = NULL, int entNum = -1, int modelNum = -1, int boltNum = -1);
	void	CreateEffect( CPrimitiveTemplate *fx, SScheduledEffect *schedFx );

public:

	CFxScheduler();

	int		RegisterEffect( const char *file, bool bHasCorrectPath = false );	// handles pre-caching

	// Nasty overloaded madness
	//rww - maybe this should be done differently.. it's more than a bit confusing.
	//Remind me when I don't have 50 files checked out.
	void	PlayEffect( int id, vec3_t org, vec3_t fwd, int vol = -1, int rad = -1, bool isPortal = false );				// builds arbitrary perp. right vector, does a cross product to define up
	void	PlayEffect( int id, vec3_t origin, matrix3_t axis, const int boltInfo=-1, CGhoul2Info_v *ghoul2 = NULL,
				int fxParm = -1, int vol = -1, int rad = -1, bool isPortal = false, int iLoopTime = false, bool isRelative = false  );
	void	PlayEffect( const char *file, vec3_t org, int vol = -1, int rad = -1 );					// uses a default up axis
	void	PlayEffect( const char *file, vec3_t org, vec3_t fwd, int vol = -1, int rad = -1 );		// builds arbitrary perp. right vector, does a cross product to define up
	void	PlayEffect( const char *file, vec3_t origin,
				matrix3_t axis, const int boltInfo = -1, CGhoul2Info_v *ghoul2 = NULL, int fxParm = -1, int vol = -1, int rad = -1, int iLoopTime = false, bool isRelative = false );

	void	StopEffect( const char *file, const int boltInfo, bool isPortal = false );	//find a scheduled Looping effect with these parms and kill it
	void	AddScheduledEffects( bool portal );								// call once per CGame frame

	// kef -- called for a 2D effect instead of addRefToScene
	bool	Add2DEffect(float x, float y, float w, float h, vec4_t color, qhandle_t shaderHandle);
	// kef -- called once per cgame frame AFTER trap->RenderScene
	void	Draw2DEffects(float screenXScale, float screenYScale);

	int		GetHighWatermark() const { return mScheduledEffectsPool.GetHighWatermark(); }
	int		NumScheduledFx()	{ return (int)mFxSchedule.size();	}
	void	Clean(bool bRemoveTemplates = true, int idToPreserve = 0);	// clean out the system

	// FX Override functions
	SEffectTemplate		*GetEffectCopy( int fxHandle, int *newHandle );
	SEffectTemplate		*GetEffectCopy( const char *file, int *newHandle );

	CPrimitiveTemplate	*GetPrimitiveCopy( SEffectTemplate *effectCopy, const char *componentName );

	void	MaterialImpact(trace_t *tr, CEffect *effect);
};

//-------------------
// The one and only
//-------------------
extern CFxScheduler theFxScheduler;
