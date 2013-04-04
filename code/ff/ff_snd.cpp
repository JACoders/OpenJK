#include "common_headers.h"

#ifdef _IMMERSION

#include "ff_snd.h"
#include "ff.h"

extern FFSystem gFFSystem;

#define FF_GAIN_STEP 500

//
//	Internal data structures
//
//	This whole system should mirror snd_dma.cpp to some degree.
//	Right now, not much works.

/*
template<typename T>
static T RelativeDistance( T Volume, T Min, T Max )
{
	if ( Min == Max )
		if ( Volume < Min )
			return 0.f;
		else
			return 1.f;

	return (Volume - Min) / (Max - Min);
};

template<typename T>
int Round( T value, int mod )
{
	int intval = (int)value;
	int intmod = intval % mod;
	int roundup = intmod >= mod / 2;

	return
	(	intval
	?	roundup
	?	intval + mod - intmod
	:	intval - intmod
	:	roundup
	?	mod
	:	0
	);
}
*/

class SndForce
{
public:
	ffHandle_t mHandle;
	int mRefs;
	qboolean mPlaying;
//	int mEntNum;
//	vec3_t mOrigin;
//	struct SDistanceLimits
//	{	int	min;
//		int max;
//	}	mDistance;

public:
	void zero()
	{
		mHandle = FF_HANDLE_NULL;
		mRefs = 0;
		mPlaying = qfalse;
//		mEntNum = 0;
//		mDistance.min = 0;
//		mDistance.max = 0;
//		mOrigin[0] = 1.f;
//		mOrigin[1] = 0.f;
//		mOrigin[2] = 0.f;
	}
	SndForce()
	{
		zero();
	}
	SndForce( const SndForce &other )
	{
		memcpy( this, &other, sizeof(SndForce) );
	}
	SndForce( ffHandle_t handle/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
	:	mHandle( handle )
	,	mRefs( 0 )
	,	mPlaying( qfalse )
//	,	mEntNum( entNum )
	{
//		mDistance.min = minDistance;
//		mDistance.max = maxDistance;
//		memcpy( mOrigin, origin, sizeof(mOrigin) );
	}
	void AddRef() 
	{
		++mRefs;
	}
	void SubRef()
	{
		--mRefs;
	}
	qboolean Update( void ) const
	{
		return qboolean
		(	mRefs != 0
		&&	(ChannelCompound*)mHandle
		);
	}
/*	int GetGain()
	{
		float distance = 1.f - GetRelativeDistance();

		return distance == 0.f
		?	10000
		:	Clamp<int>
			(	Round<int>
				(	distance * 10000
				,	FF_GAIN_STEP
				)
			,	0
			,	10000
			)
		;
	}
	float GetRelativeDistance()
	{
		return !mRefs
		?	1.f
		:	IsOrigin()
		?	0.f
		:	RelativeDistance<float>
			(	sqrt
				(	mOrigin[0] * mOrigin[0]
				+	mOrigin[1] * mOrigin[1]
				+	mOrigin[2] * mOrigin[2]
				)
			,	mDistance.min
			,	mDistance.max
			) / mRefs
		;
	}
	qboolean IsOrigin()
	{
		return qboolean
		(	!mOrigin[0]
		&&	!mOrigin[1]
		&&	!mOrigin[2]
		);
	}
	void Respatialize( int entNum, const vec3_t origin )
	{
		extern vec3_t s_entityPosition[];

		if ( mEntNum != entNum )
		{
			// Assumes all forces follow its entity and is centered on entity
			mOrigin[0] = s_entityPosition[ entNum ][0] - origin[0];
			mOrigin[1] = s_entityPosition[ entNum ][1] - origin[1];
			mOrigin[2] = s_entityPosition[ entNum ][2] - origin[2];
		}
		else
		{
			memset( mOrigin, 0, sizeof(mOrigin) );
		}
	}*/
	void operator += ( SndForce &other );
};

// Fancy comparator
struct SndForceLess : public less<SndForce>
{
	bool operator() ( const SndForce &x, const SndForce &y )
	{
		return bool
		(/* x.mEntNum < y.mEntNum
		||*/x.mHandle < y.mHandle
//		||	x.mOrigin < y.mOrigin		// uhhh... compare components
		||	x.mPlaying < y.mPlaying
		);
	}
};


class LoopForce : public SndForce
{
public:
	LoopForce(){}
	LoopForce( const LoopForce &other )
	{
		memcpy( this, &other, sizeof(LoopForce) );
	}
	LoopForce( ffHandle_t handle/*int entNum, , const vec3_t origin, float maxDistance, float minDistance*/ )
	:	SndForce( handle/*, entNum, origin, maxDistance, minDistance*/ )
	{}

	void Add( ffHandle_t ff/*, int entNum, const vec3_t origin*/ );
//	void Respatialize( int entNum, const vec3_t origin );
	qboolean Update( void )
	{
		qboolean result = SndForce::Update();
		mRefs = 0;
		return result;
	}
};

class SndForceSet
{
public:
	typedef set<SndForce, SndForceLess> PendingSet;
	typedef map<ffHandle_t, SndForce> ActiveSet;
	ActiveSet mActive;
	PendingSet mPending;
public:
	void Add( ffHandle_t handle/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
	{
		const_cast <SndForce&> (*mPending.insert( SndForce( handle/*, entNum, origin, maxDistance, minDistance*/ ) ).first).AddRef();
	}
	qboolean Update( void );
/*	void Respatialize( int entNum, const vec3_t origin )
	{
		for
		(	PendingSet::iterator itPending = mPending.begin()
		;	itPending != mPending.end()
		;	itPending++
		){
			(*itPending).Respatialize( entNum, origin );
		}
	}*/
};

class LoopForceSet 
{
public:
	typedef set<LoopForce, SndForceLess> PendingSet;
	typedef map<ffHandle_t, LoopForce> ActiveSet;
	ActiveSet mActive;
	PendingSet mPending;
public:
	void Add( ffHandle_t handle/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
	{
		const_cast <LoopForce&>(*mPending.insert( LoopForce( handle/*, entNum, origin, maxDistance, minDistance*/ ) ).first).AddRef();
	}
	qboolean Update( void );
/*	void Respatialize( int entNum, const vec3_t origin )
	{
		for
		(	PendingSet::iterator itPending = mPending.begin()
		;	itPending != mPending.end()
		;	itPending++
		){
			(*itPending).Respatialize( entNum, origin );
		}
	}*/
};

class MasterForceSet
{
protected:
	int mEntityNum;
//	vec3_t mOrigin;
	SndForceSet mSnd;
	LoopForceSet mLoop;

public:
	void Add( ffHandle_t handle/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
	{
		mSnd.Add( handle/*, entNum, origin, maxDistance, minDistance*/ );
	}
	void AddLoop( ffHandle_t handle/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
	{
		mLoop.Add( handle/*, entNum, origin, maxDistance, minDistance*/ );
	}
/*	void Respatialize( int entNum, const vec3_t origin )
	{
		memcpy( mOrigin, origin, sizeof(mOrigin) );
		mEntityNum = entNum;
		mSnd.Respatialize( entNum, origin );
		mLoop.Respatialize( entNum, origin );
	}
*/	void Update( void );
};

// 
//	===================================================================================
//

static MasterForceSet _MasterForceSet;

void FF_AddForce( ffHandle_t ff/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
{
	_MasterForceSet.Add( ff/*, entNum, origin, maxDistance, minDistance*/ );
}

void FF_AddLoopingForce( ffHandle_t ff/*, int entNum, const vec3_t origin, float maxDistance, float minDistance*/ )
{
	_MasterForceSet.AddLoop( ff/*, entNum, origin, maxDistance, minDistance*/ );
}
/*
void FF_Respatialize( int entNum, const vec3_t origin )
{
	_MasterForceSet.Respatialize( entNum, origin );
}
*/
void FF_Update( void )
{
	_MasterForceSet.Update();
}

//
//	===================================================================================
//

void MasterForceSet::Update()
{
	mSnd.Update();
	mLoop.Update();
}

////-----------------
/// LoopForce::Update
//---------------------
//	Starts/Stops/Updates looping forces.
//	Call once per frame after all looping forces have been added and respatialized.
//
qboolean LoopForceSet::Update()
{
	ActiveSet::iterator itActive;
	PendingSet::iterator itPending;

	// Sum effects
	ActiveSet active;
	for
	(	itPending = mPending.begin()
	;	itPending != mPending.end()
	;	itPending++
	){
		if ( (const_cast <LoopForce&> (*itPending)).Update() )
		{
			active[ (*itPending).mHandle ] += const_cast <LoopForce&> (*itPending) ;
		}
	}

	// Stop and remove unreferenced effects 
	for
	(	itActive = mActive.begin()
	;	itActive != mActive.end()
	;	//itActive++
	){
		if ( active.find( (*itActive).first ) != active.end() )
		{
			itActive++;
		}
		else
		{
			SndForce &sndForce = (*itActive).second;
			FF_Stop( sndForce.mHandle );
			itActive = mActive.erase( itActive );
		}
	}

	// Decide whether to start or update
	for
	(	itActive = active.begin()
	;	itActive != active.end()
	;	itActive++
	){
		SndForce &sndForce = mActive[ (*itActive).first ];
		sndForce.mHandle = (*itActive).first;
		if ( sndForce.mPlaying )
		{
			// Just update it

//			if ( (*itActive).second.GetGain() != sndForce.GetGain() )
//			{
//				gFFSystem.ChangeGain( sndForce.mHandle, sndForce.GetGain() );
//			}
		}
		else
		{
			// Update and start it

//			gFFSystem.ChangeGain( sndForce.mHandle, sndForce.GetGain() );
			FF_Play( sndForce.mHandle );
			sndForce.mPlaying = qtrue;
		}
	}

	mPending.clear();

	return qtrue;
}

////-------------------
/// SndForceSet::Update
//-----------------------
//
//
qboolean SndForceSet::Update()
{
	ActiveSet::iterator itActive;
	PendingSet::iterator itPending;
/*
	// Remove finished effects from active //and pending sets
	for
	(	itActive = mActive.begin()
	;	itActive != mActive.end()
	;	//itActive++
	){
		if ( gFFSystem.IsPlaying( (*itActive).first ) )
		{
			++itActive;
		}
		else
		{
#if( 0 )
			for
			(	itPending = mPending.begin()
			;	itPending != mPending.end()
			;	itPending++
			){
				if 
				(	(*itPending).mHandle == (*itActive).first
				&&	(*itPending).mPlaying
				){
					itPending = mPending.erase( itPending );
				}
			}
#endif
			itActive = mActive.erase( itActive );
		}
	}
*/
	// Sum effects
	ActiveSet start; 
	for
	(	itPending = mPending.begin()
	;	itPending != mPending.end()
	;	itPending++
	){
		if ( (*itPending).Update() )
		{
			start[ (*itPending).mHandle ] += const_cast <SndForce&> (*itPending);
		}
	}

	// Decide whether to start ( no updating one-shots )
	for
	(	itActive = start.begin()
	;	itActive != start.end()
	;	itActive++
	){
/*		SndForce &sndForce = mActive[ (*itActive).first ];
		sndForce.mHandle = (*itActive).first;
		if ( (*itActive).second.GetGain() >= sndForce.GetGain() )
		{
			//gFFSystem.ChangeGain( sndForce.mHandle, sndForce.GetGain() );
			FF_Start( sndForce.mHandle );
			sndForce.mPlaying = qtrue;
		}
*/		FF_Play( (*itActive).first );
	}

	mPending.clear();

	return qfalse;
}

void SndForce::operator += ( SndForce &other )
{
	/*
	float dist = other.GetRelativeDistance();

	if ( dist < 1.f )
	{
		float thisdist = GetRelativeDistance();

		if ( thisdist < 1.f )
		{
			if ( dist == 0.f || thisdist == 0.f )
			{
				mOrigin[0] = 0.f;
				mOrigin[1] = 0.f;
				mOrigin[2] = 0.f;
			}
			else
			{
				// This is so shitty
				mOrigin[0] *= dist;
				mOrigin[1] *= dist;
				mOrigin[2] *= dist;
			}
		}
		else
		{
			memcpy( mOrigin, other.mOrigin, sizeof(mOrigin) );
		}
	*/

		mRefs += other.mRefs;
//	}
}

#endif // _IMMERSION