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

#include "FxSystem.h"

#define MAX_EFFECTS			1800

// Generic group flags, used by parser, then get converted to the appropriate specific flags
#define FX_PARM_MASK		0xC	// use this to mask off any transition types that use a parm
#define FX_GENERIC_MASK		0xF
#define FX_LINEAR			0x1
#define FX_RAND				0x2
#define FX_NONLINEAR		0x4
#define FX_WAVE				0x8
#define FX_CLAMP			0xC

// Group flags
#define FX_ALPHA_SHIFT		0
#define FX_ALPHA_PARM_MASK	0x0000000C
#define	FX_ALPHA_LINEAR		0x00000001
#define FX_ALPHA_RAND		0x00000002
#define	FX_ALPHA_NONLINEAR	0x00000004
#define FX_ALPHA_WAVE		0x00000008
#define FX_ALPHA_CLAMP		0x0000000C

#define FX_RGB_SHIFT		4
#define FX_RGB_PARM_MASK	0x000000C0
#define	FX_RGB_LINEAR		0x00000010
#define FX_RGB_RAND			0x00000020
#define	FX_RGB_NONLINEAR	0x00000040
#define FX_RGB_WAVE			0x00000080
#define FX_RGB_CLAMP		0x000000C0

#define FX_SIZE_SHIFT		8
#define FX_SIZE_PARM_MASK	0x00000C00
#define	FX_SIZE_LINEAR		0x00000100
#define FX_SIZE_RAND		0x00000200
#define	FX_SIZE_NONLINEAR	0x00000400
#define FX_SIZE_WAVE		0x00000800
#define FX_SIZE_CLAMP		0x00000C00

#define FX_LENGTH_SHIFT		12
#define FX_LENGTH_PARM_MASK	0x0000C000
#define	FX_LENGTH_LINEAR	0x00001000
#define FX_LENGTH_RAND		0x00002000
#define	FX_LENGTH_NONLINEAR	0x00004000
#define FX_LENGTH_WAVE		0x00008000
#define FX_LENGTH_CLAMP		0x0000C000

#define FX_SIZE2_SHIFT		16
#define FX_SIZE2_PARM_MASK	0x000C0000
#define	FX_SIZE2_LINEAR		0x00010000
#define FX_SIZE2_RAND		0x00020000
#define	FX_SIZE2_NONLINEAR	0x00040000
#define FX_SIZE2_WAVE		0x00080000
#define FX_SIZE2_CLAMP		0x000C0000

// Shared flag--these flags, at first glance would appear to be shared, but are safe.  I'd rather not do this, but as you can see, features flags are currently all accounted for
#define	FX_PAPER_PHYSICS	0x00010000		// emitters ONLY.  shared with FX_SIZE_2_LINEAR
#define	FX_LOCALIZED_FLASH	0x00010000		// full screen flashes ONLY.  shared with FX_SIZE_2_LINEAR
#define	FX_PLAYER_VIEW		0x00010000		// player view effects ONLY.  shared with FX_SIZE_2_LINEAR

// Feature flags
#define	FX_DEPTH_HACK		0x00100000
#define	FX_RELATIVE			0x00200000
#define FX_SET_SHADER_TIME	0x00400000
#define FX_EXPENSIVE_PHYSICS 0x00800000

//rww - g2-related flags (these can slow things down significantly, use sparingly)
//These should be used only with particles/decals as they steal flags used by cylinders.
#define	FX_GHOUL2_TRACE		0x00020000		//use in conjunction with particles - actually do full ghoul2 traces for physics collision against entities with a ghoul2 instance
							//shared FX_SIZE2_RAND (used only with cylinders)
#define	FX_GHOUL2_DECALS	0x00040000		//use in conjunction with decals - can project decal as a ghoul2 gore skin object onto ghoul2 models
							//shared FX_SIZE2_NONLINEAR (used only with cylinders)

#define FX_ATTACHED_MODEL	0x01000000

#define FX_APPLY_PHYSICS	0x02000000
#define FX_USE_BBOX			0x04000000		// can make physics more accurate at the expense of speed

#define FX_USE_ALPHA		0x08000000		// the FX system actually uses RGB to do fades, but this will override that
											//	and cause it to fill in the alpha.

#define FX_EMIT_FX			0x10000000		// emitters technically don't have to emit stuff, but when they do
											//	this flag needs to be set
#define FX_DEATH_RUNS_FX	0x20000000		// Normal death triggers effect, but not kill_on_impact
#define FX_KILL_ON_IMPACT	0x40000000		// works just like it says, but only when physics are on.
#define FX_IMPACT_RUNS_FX	0x80000000		// an effect can call another effect when it hits something.

// Lightning flags, duplicates of existing flags, but lightning doesn't use those flags in that context...and nothing will ever use these in this context..so we are safe.
#define FX_TAPER			0x01000000		// tapers as it moves towards its endpoint
#define FX_BRANCH			0x02000000		// enables lightning branching
#define FX_GROW				0x04000000		// lightning grows from start point to end point over the course of its life


// stuff that can occur when an effect is flagged with "materialImpact" and it hits something
enum EMatImpactEffect
{
	MATIMPACTFX_NONE = 0,
	MATIMPACTFX_SHELLSOUND
};

//------------------------------
class CEffect
{
protected:

	CEffect		*mNext;
	vec3_t		mOrigin1;

	int			mTimeStart;
	int			mTimeEnd;

	unsigned int	mFlags;

	EMatImpactEffect	mMatImpactFX;
	int					mMatImpactParm;

	// Size of our object, useful for things that have physics
	vec3_t		mMin;
	vec3_t		mMax;

	int			mImpactFxID;		// if we have an impact event, we may have to call an effect
	int			mDeathFxID;			// if we have a death event, we may have to call an effect

	miniRefEntity_t		mRefEnt;

	int			mSoundRadius;
	int			mSoundVolume;

public:

	CEffect();
	virtual ~CEffect() {}

	virtual void Die() {}
	virtual bool Update()	{ return true;		}
	virtual	void Draw(void) {}

	inline	miniRefEntity_t	&GetRefEnt(void) { return mRefEnt; }

	inline void		SetNext(CEffect *Next) { mNext = Next; }
	inline CEffect	*GetNext(void) { return mNext; }
	inline void		GetOrigin(vec3_t dest)		{VectorCopy( mOrigin1, dest); }

	inline void SetSTScale(float s,float t)	{ mRefEnt.shaderTexCoord[0]=s;mRefEnt.shaderTexCoord[1]=t;}

	inline void SetSound ( int vol, int rad) { mSoundRadius = rad; mSoundVolume = vol; }
	inline void SetMin( vec3_t min )		{ if(min){VectorCopy(min,mMin);}else{VectorClear(mMin);}			}
	inline void SetMax( vec3_t max )		{ if(max){VectorCopy(max,mMax);}else{VectorClear(mMax);}			}
	inline void SetFlags( int flags )		{ mFlags = flags;				}
	inline void AddFlags( int flags )		{ mFlags |= flags;				}
	inline void ClearFlags( int flags )		{ mFlags &= ~flags;				}
	inline void SetOrigin1( vec3_t org )	{ if(org){VectorCopy(org,mOrigin1);}else{VectorClear(mOrigin1);}	}
	inline void SetTimeStart( int time )	{ mTimeStart = time; if (mFlags&FX_SET_SHADER_TIME) { mRefEnt.shaderTime = time * 0.001f; }}
	inline void	SetTimeEnd( int time )		{ mTimeEnd = time;				}
	inline void SetImpactFxID( int id )		{ mImpactFxID = id;				}
	inline void SetDeathFxID( int id )		{ mDeathFxID = id;				}
	inline EMatImpactEffect GetMatImpactFX() { return mMatImpactFX; }
	inline int	GetMatImpactParm() { return mMatImpactParm; }
	inline void SetMatImpactFX(EMatImpactEffect matFX) { mMatImpactFX = matFX; }
	inline void SetMatImpactParm(int matParm) { mMatImpactParm = matParm; }
};

//---------------------------------------------------
// This class is kind of an exception to the "rule".
//	For now it exists only for allowing an easy way
//	to get the saber slash trails rendered.
//---------------------------------------------------
class CTrail : public CEffect
{
// This is such a specific case thing, just grant public access to the goods.
protected:

	void Draw();

public:

	typedef struct
	{
		vec3_t	origin;

		// very specifc case, we can modulate the color and the alpha
		vec3_t	rgb;
		vec3_t	destrgb;
		vec3_t	curRGB;

		float	alpha;
		float	destAlpha;
		float	curAlpha;

		// this is a very specific case thing...allow interpolating the st coords so we can map the texture
		//	properly as this segement progresses through it's life
		float	ST[2];
		float	destST[2];
		float	curST[2];

	} TVert;

	TVert		mVerts[4];
	qhandle_t	mShader;


	CTrail()			{};
	virtual ~CTrail()	{};

	virtual bool Update();
};

//------------------------------
class CLight : public CEffect
{
protected:

	float		mSizeStart;
	float		mSizeEnd;
	float		mSizeParm;

	vec3_t		mOrgOffset;
	vec3_t		mRGBStart;
	vec3_t		mRGBEnd;
	float		mRGBParm;

	CGhoul2Info_v *mGhoul2;
	short		mEntNum;
	char		mModelNum;
	char		mBoltNum;

	void		UpdateSize();
	void		UpdateRGB();

	virtual void		Draw(void);
public:

	inline CLight(void)
	{
		mEntNum = -1; mModelNum = -1; mBoltNum = -1;
	}

	inline void SetBoltinfo( CGhoul2Info_v *ghoul2,  int entNum, int modelNum = -1, int boltNum = -1 )
	{
		mGhoul2 = ghoul2;
		mEntNum = entNum;
		mModelNum = modelNum;
		mBoltNum = boltNum;
	}

	virtual bool Update();

	inline void SetSizeStart( float sz )	{ mSizeStart = sz;			}
	inline void SetSizeEnd( float sz )		{ mSizeEnd = sz;			}
	inline void SetSizeParm( float parm )	{ mSizeParm = parm;			}

	inline void SetOrgOffset( const vec3_t o )	{ if(o){VectorCopy(o,mOrgOffset);}else{VectorClear(mOrgOffset);}}
	inline void SetRGBStart( vec3_t rgb )	{ if(rgb){VectorCopy(rgb,mRGBStart);}else{VectorClear(mRGBStart);}	}
	inline void SetRGBEnd( vec3_t rgb )		{ if(rgb){VectorCopy(rgb,mRGBEnd);}else{VectorClear(mRGBEnd);}		}
	inline void SetRGBParm( float parm )	{ mRGBParm = parm;			}
};

//------------------------------
class CParticle : public CEffect
{
protected:

	vec3_t		mOrgOffset;

	vec3_t		mVel;
	vec3_t		mAccel;

	float		mSizeStart;
	float		mSizeEnd;
	float		mSizeParm;

	vec3_t		mRGBStart;
	vec3_t		mRGBEnd;
	float		mRGBParm;

	float		mAlphaStart;
	float		mAlphaEnd;
	float		mAlphaParm;

	float		mRotationDelta;
	float		mElasticity;

	CGhoul2Info_v *mGhoul2;
	short		mEntNum;
	char		mModelNum;
	char		mBoltNum;

	bool		UpdateOrigin();
	void		UpdateSize();
	void		UpdateRGB();
	void		UpdateAlpha();
	void		UpdateRotation();


public:

	inline void SetBoltinfo( CGhoul2Info_v *ghoul2,  int entNum, int modelNum = -1, int boltNum = -1 )
	{
		mGhoul2 = ghoul2;
		mEntNum = entNum;
		mModelNum = modelNum;
		mBoltNum = boltNum;
	}

	inline CParticle(void)
	{
		mRefEnt.reType = RT_SPRITE; mEntNum = -1; mModelNum = -1; mBoltNum = -1;
	}

	virtual void Init();
	virtual void Die();
	virtual bool Update();
	virtual bool Cull(void);
	virtual void Draw(void);

	inline void SetShader( qhandle_t sh )	{ mRefEnt.customShader = sh;		}

	inline void SetOrgOffset( const vec3_t o )	{ if(o){VectorCopy(o,mOrgOffset);}else{VectorClear(mOrgOffset);}}
	inline void SetVel( vec3_t vel )		{ if(vel){VectorCopy(vel,mVel);}else{VectorClear(mVel);}	}
	inline void SetAccel( vec3_t ac )		{ if(ac){VectorCopy(ac,mAccel);}else{VectorClear(mAccel);}	}

	inline void SetSizeStart( float sz )	{ mSizeStart = sz; mRefEnt.radius = sz;	}
	inline void SetSizeEnd( float sz )		{ mSizeEnd = sz;			}
	inline void SetSizeParm( float parm )	{ mSizeParm = parm;			}

	inline void SetRGBStart( vec3_t rgb )	{ if(rgb){VectorCopy(rgb,mRGBStart);}else{VectorClear(mRGBStart);}	}
	inline void SetRGBEnd( vec3_t rgb )		{ if(rgb){VectorCopy(rgb,mRGBEnd);}else{VectorClear(mRGBEnd);}		}
	inline void SetRGBParm( float parm )	{ mRGBParm = parm;			}

	inline void SetAlphaStart( float al )	{ mAlphaStart = al;			}
	inline void SetAlphaEnd( float al )		{ mAlphaEnd = al;			}
	inline void SetAlphaParm( float parm )	{ mAlphaParm = parm;		}

	inline void SetRotation( float rot )		{ mRefEnt.rotation = rot;	}
	inline void SetRotationDelta( float rot )	{ mRotationDelta = rot;		}
	inline void SetElasticity( float el )		{ mElasticity = el;			}
};

//------------------------------
class CFlash : public CParticle
{
public:

	CFlash():
	mScreenX(0),
	mScreenY(0),
	mRadiusModifier(1)
	{}

	virtual ~CFlash() {}

	virtual bool Update();
	virtual void Draw(void);
	virtual bool Cull(void) { return false; }

	void		Init( void );

protected:
	// kef -- mScreenX and mScreenY are used for flashes that are FX_LOCALIZED_FLASH
	float		mScreenX;
	float		mScreenY;
	float		mRadiusModifier;
};

//------------------------------
class CLine : public CParticle
{
protected:

	vec3_t	mOrigin2;

	virtual void		Draw(void);
public:

	CLine();
	virtual ~CLine() {}

	virtual void Die() {}

	virtual bool Update();

	inline void SetOrigin2( vec3_t org2 )	{ VectorCopy( org2, mOrigin2 ); }
};

//------------------------------
class CBezier : public CLine
{
protected:

	vec3_t	mControl1;
	vec3_t	mControl1Vel;

	vec3_t	mControl2;
	vec3_t	mControl2Vel;

	bool	mInit;

public:

	CBezier(){ mInit = false; }
	virtual ~CBezier() {}

	virtual void Die() {}
	virtual bool Update();
	virtual bool Cull(void);
	virtual void Draw(void);

	void DrawSegment( vec3_t start, vec3_t end, float texcoord1, float texcoord2, float segPercent, float lastSegPercent );

	inline void SetControlPoints( vec3_t ctrl1, vec3_t ctrl2 )	{ VectorCopy( ctrl1, mControl1 ); VectorCopy( ctrl2, mControl2 ); }
	inline void SetControlVel( vec3_t ctrl1v, vec3_t ctrl2v )	{ VectorCopy( ctrl1v, mControl1Vel ); VectorCopy( ctrl2v, mControl2Vel ); }
};

//------------------------------
class CElectricity : public CLine
{
protected:

	float	mChaos;

	virtual void		Draw(void);
public:

	CElectricity();
	virtual ~CElectricity() {}

	virtual void Die() {}

	virtual bool Update();

	void Initialize();

	inline void SetChaos( float chaos )		{ mChaos = chaos; }
};


// Oriented quad
//------------------------------
class COrientedParticle : public CParticle
{
protected:

	vec3_t	mNormal;

public:

	COrientedParticle();
	virtual ~COrientedParticle() {}

	virtual bool Update();
	virtual bool Cull(void);
	virtual void Draw(void);

	inline void SetNormal( vec3_t norm )	{ VectorCopy( norm, mNormal );	}
};

//------------------------------
class CTail : public CParticle
{
protected:

	vec3_t	mOldOrigin;

	float	mLengthStart;
	float	mLengthEnd;
	float	mLengthParm;

	float	mLength;

	void	UpdateLength();
	void	CalcNewEndpoint();

	virtual void		Draw(void);
public:

	CTail();
	virtual ~CTail() {}

	virtual bool Update();

	inline void SetLengthStart( float len )	{ mLengthStart = len;	}
	inline void SetLengthEnd( float len )	{ mLengthEnd = len;	}
	inline void SetLengthParm( float len )	{ mLengthParm = len;	}
};


//------------------------------
class CCylinder : public CTail
{
protected:

	float		mSize2Start;
	float		mSize2End;
	float		mSize2Parm;
	qboolean	mTraceEnd;

	void	UpdateSize2();

	virtual void		Draw(void);
public:

	CCylinder();
	virtual ~CCylinder() {}

	virtual bool Cull(void);
	virtual void UpdateLength(void);
	virtual bool Update();

	inline void SetSize2Start( float sz )	{ mSize2Start = sz;			}
	inline void SetSize2End( float sz )		{ mSize2End = sz;			}
	inline void SetSize2Parm( float parm )	{ mSize2Parm = parm;		}
	inline void SetTraceEnd(qboolean traceEnd) { mTraceEnd = traceEnd; }

	inline void SetNormal( vec3_t norm )	{ VectorCopy( norm, mRefEnt.axis[0] ); }
};


//------------------------------
// Emitters are derived from particles because, although they don't draw, any effect called
//	from them can borrow an initial or ending value from the emitters current alpha, rgb, etc..
class CEmitter : public CParticle
{
protected:

	vec3_t		mOldOrigin;		// we use these to do some nice
	vec3_t		mLastOrigin;	//	tricks...
	vec3_t		mOldVelocity;	//
	int			mOldTime;

	vec3_t		mAngles;		// for a rotating thing, using a delta
	vec3_t		mAngleDelta;	//	as opposed to an end angle is probably much easier

	int			mEmitterFxID;	// if we have emitter fx, this is our id

	float		mDensity;		// controls how often emitter chucks an effect
	float		mVariance;		// density sloppiness

	void		UpdateAngles();

	virtual void		Draw(void);
public:

	CEmitter();
	virtual ~CEmitter();

	virtual bool Cull(void) {return false;}
	virtual bool Update();

	inline void SetModel( qhandle_t model )	{ mRefEnt.hModel = model;	}
	inline void SetAngles( vec3_t ang )		{ if(ang){VectorCopy(ang,mAngles);}else{VectorClear(mAngles);}				}
	inline void SetAngleDelta( vec3_t ang )	{ if(ang){VectorCopy(ang,mAngleDelta);}else{VectorClear(mAngleDelta);}		}
	inline void SetEmitterFxID( int id )	{ mEmitterFxID = id;		}
	inline void SetDensity( float density )	{ mDensity = density;		}
	inline void SetVariance( float var )	{ mVariance = var;			}
	inline void SetOldTime( int time )		{ mOldTime = time;			}
	inline void SetLastOrg( vec3_t org )	{ if(org){VectorCopy(org,mLastOrigin);}else{VectorClear(mLastOrigin);}		}
	inline void SetLastVel( vec3_t vel )	{ if(vel){VectorCopy(vel,mOldVelocity);}else{VectorClear(mOldVelocity);}	}
};

// We're getting pretty low level here, not the kind of thing to abuse considering how much overhead this
//	adds to a SINGLE triangle or quad....
//------------------------------
#define MAX_CPOLY_VERTS	5

class CPoly : public CParticle
{
protected:

	int		mCount;
	vec3_t	mRotDelta;
	int		mTimeStamp;

public:

	vec3_t	mOrg[MAX_CPOLY_VERTS];
	vec2_t	mST[MAX_CPOLY_VERTS];

	float	mRot[3][3];
	int		mLastFrameTime;


	CPoly() {}
	virtual ~CPoly() {}

	virtual bool Update();
	virtual bool Cull(void);
	virtual void Draw(void);

	void PolyInit();
	void CalcRotateMatrix();
	void Rotate();

	inline void SetNumVerts( int c )					{ mCount = c;			}
	inline void SetRot( vec3_t r )						{ if(r){VectorCopy(r,mRotDelta);}else{VectorClear(mRotDelta);}}
	inline void SetMotionTimeStamp( int t )				{ mTimeStamp = theFxHelper.GetTime() + t; }
	inline int	GetMotionTimeStamp()					{ return mTimeStamp; }
};
