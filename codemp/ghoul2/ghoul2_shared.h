#pragma once

/*
Ghoul2 Insert Start
*/
#ifdef _MSC_VER
#pragma warning (push, 3)	//go back down to 3 for the stl include
#endif
#include <vector>
#include <map>
#ifdef _MSC_VER
#pragma warning (pop)
#endif
using namespace std;
/*
Ghoul2 Insert End
*/

#define MDXABONEDEF
#include "rd-common/mdx_format.h"
#include "rd-common/tr_types.h"
#include "qcommon/matcomp.h"
#include "ghoul2/G2_gore.h"

struct model_s;

//rww - RAGDOLL_BEGIN
#define G2T_SV_TIME (0)
#define G2T_CG_TIME (1)
#define NUM_G2T_TIME (2)

//rww - RAGDOLL_END

//===================================================================
//
//   G H O U L  I I  D E F I N E S
//
// we save the whole surfaceInfo_t struct
struct surfaceInfo_t 
{
	int			offFlags;		// what the flags are for this model
	int			surface;		// index into array held inside the model definition of pointers to the actual surface data loaded in - used by both client and game
	float		genBarycentricJ;	// point 0 barycentric coors
	float		genBarycentricI;	// point 1 barycentric coors - point 2 is 1 - point0 - point1
	int			genPolySurfaceIndex; // used to point back to the original surface and poly if this is a generated surface
	int			genLod;			// used to determine original lod of original surface and poly hit location
	
surfaceInfo_t():
	offFlags(0),
	surface(0),
	genBarycentricJ(0),
	genBarycentricI(0),
	genPolySurfaceIndex(0),
	genLod(0)
	{}

};



#define MDXABONEDEF				// used in the mdxformat.h file to stop redefinitions of the bone struct.

// we save the whole structure here.
struct  boneInfo_t
{
	int			boneNumber;		// what bone are we overriding?
	mdxaBone_t	matrix;			// details of bone angle overrides - some are pre-done on the server, some in ghoul2
	int			flags;			// flags for override
	int			startFrame;		// start frame for animation
	int			endFrame;		// end frame for animation NOTE anim actually ends on endFrame+1
	int			startTime;		// time we started this animation
	int			pauseTime;		// time we paused this animation - 0 if not paused
	float		animSpeed;		// speed at which this anim runs. 1.0f means full speed of animation incoming - ie if anim is 20hrtz, we run at 20hrts. If 5hrts, we run at 5 hrts
	float		blendFrame;		// frame PLUS LERP value to blend from
	int			blendLerpFrame;	// frame to lerp the blend frame with.
	int			blendTime;		// Duration time for blending - used to calc amount each frame of new anim is blended with last frame of the last anim
	int			blendStart;		// Time when blending starts - not necessarily the same as startTime since we might start half way through an anim
	int			boneBlendTime;	// time for duration of bone angle blend with normal animation
	int			boneBlendStart;	// time bone angle blend with normal animation began
	int			lastTime;		// this does not go across the network
	mdxaBone_t	newMatrix;		// This is the lerped matrix that Ghoul2 uses on the client side - does not go across the network

	//rww - RAGDOLL_BEGIN
	int			lastTimeUpdated;  // if non-zero this is all intialized
	int			lastContents;
	vec3_t		lastPosition;
	vec3_t		velocityEffector;
	vec3_t		lastAngles;
	vec3_t		minAngles;
	vec3_t		maxAngles;
	vec3_t		currentAngles;
	vec3_t		anglesOffset;
	vec3_t		positionOffset;
	float		radius;
	float		weight;   // current radius cubed
	int			ragIndex;
	vec3_t		velocityRoot;		// I am really tired of recomiling the whole game to add a param here
	int			ragStartTime;
	int			firstTime;
	int			firstCollisionTime;
	int			restTime;
	int			RagFlags;
	int			DependentRagIndexMask;
	mdxaBone_t	originalTrueBoneMatrix;	
	mdxaBone_t	parentTrueBoneMatrix;			// figure I will need this sooner or later
	mdxaBone_t	parentOriginalTrueBoneMatrix;	// figure I will need this sooner or later
	vec3_t		originalOrigin;
	vec3_t		originalAngles;
	vec3_t		lastShotDir;		
	mdxaBone_t  *basepose;
	mdxaBone_t  *baseposeInv;
	mdxaBone_t  *baseposeParent;
	mdxaBone_t  *baseposeInvParent;
	int			parentRawBoneIndex;
	mdxaBone_t	ragOverrideMatrix;	// figure I will need this sooner or later

	mdxaBone_t	extraMatrix;	// figure I will need this sooner or later
	vec3_t		extraVec1;		// I am really tired of recomiling the whole game to add a param here
	float		extraFloat1;		
	int			extraInt1;

	vec3_t		ikPosition;
	float		ikSpeed;

	vec3_t		epVelocity; //velocity factor, can be set, and is also maintained by physics based on gravity, mass, etc.
	float		epGravFactor; //gravity factor maintained by bone physics
	int			solidCount; //incremented every time we try to move and are in solid - if we get out of solid, it is reset to 0
	bool		physicsSettled; //true when the bone is on ground and finished bouncing, etc. but may still be pushed into solid by other bones
	bool		snapped; //the bone is broken out of standard constraints

	int			parentBoneIndex;

	float		offsetRotation;

	//user api overrides
	float		overGradSpeed;

	vec3_t		overGoalSpot;
	bool		hasOverGoal;

	mdxaBone_t	animFrameMatrix; //matrix for the bone in the desired settling pose -rww
	int			hasAnimFrameMatrix;

	int			airTime; //base is in air, be more quick and sensitive about collisions
	//rww - RAGDOLL_END

boneInfo_t():
	boneNumber(-1),
	flags(0),
	startFrame(0),
	endFrame(0),
	startTime(0),
	pauseTime(0),
	animSpeed(0),
	blendFrame(0),
	blendLerpFrame(0),
	blendTime(0),
	blendStart(0),
	boneBlendTime(0),
	boneBlendStart(0),
	lastTime(0),
	RagFlags(0)
	{
		matrix.matrix[0][0] = matrix.matrix[0][1] = matrix.matrix[0][2] = matrix.matrix[0][3] = 
		matrix.matrix[1][0] = matrix.matrix[1][1] = matrix.matrix[1][2] = matrix.matrix[1][3] = 
		matrix.matrix[2][0] = matrix.matrix[2][1] = matrix.matrix[2][2] = matrix.matrix[2][3] = 0.0f;
	}

};
//we save from top to boltUsed here. Don't bother saving the position, it gets rebuilt every frame anyway
struct boltInfo_t{
	int			boneNumber;		// bone number bolt attaches to	
	int			surfaceNumber;	// surface number bolt attaches to 
	int			surfaceType;	// if we attach to a surface, this tells us if it is an original surface or a generated one - doesn't go across the network
	int			boltUsed;		// nor does this
	mdxaBone_t	position;		// this does not go across the network
	boltInfo_t():
	boneNumber(-1),
	surfaceNumber(-1),
	surfaceType(0),
	boltUsed(0)
	{}
};

#ifdef _SOF2
typedef enum
{
	PGORE_NONE,
	PGORE_ARMOR,
	PGORE_BULLETSMALL,
	PGORE_BULLETMED,
	PGORE_BULLETBIG,
	PGORE_HEGRENADE,
	PGORE_COUNT
} goreEnum_t;

struct goreEnumShader_t
{
	goreEnum_t		shaderEnum;
	char			shaderName[MAX_QPATH];
};

struct SSkinGoreData
{
	vec3_t			angles;
	vec3_t			position;
	int				currentTime;
	int				entNum;
	vec3_t			rayDirection;	// in world space
	vec3_t			hitLocation;	// in world space
	vec3_t			scale;
	float			SSize;			// size of splotch in the S texture direction in world units
	float			TSize;			// size of splotch in the T texture direction in world units
	float			theta;			// angle to rotate the splotch

//	qhandle_t		shader;			// handle to shader for gore, this better be rendered after the shader of the underlying surface					
									// this shader should also have "clamp" mode, not tiled.
	goreEnum_t		shaderEnum;		// enum that'll get switched over to the shader's actual handle
};
#endif // _SOF2

#define MAX_GHOUL_COUNT_BITS 8 // bits required to send across the MAX_G2_MODELS inside of the networking - this is the only restriction on ghoul models possible per entity

typedef vector <surfaceInfo_t> surfaceInfo_v;
typedef vector <boneInfo_t> boneInfo_v;
typedef vector <boltInfo_t> boltInfo_v;
typedef vector <pair<int,mdxaBone_t> > mdxaBone_v;

// defines for stuff to go into the mflags
#define		GHOUL2_NOCOLLIDE 0x001
#define		GHOUL2_NORENDER	 0x002
#define		GHOUL2_NOMODEL	 0x004
#define		GHOUL2_NEWORIGIN 0x008

//for transform optimization -rww
#define		GHOUL2_ZONETRANSALLOC	0x2000

class CBoneCache;

// NOTE order in here matters. We save out from mModelindex to mFlags, but not the STL vectors that are at the top or the bottom.
class CGhoul2Info
{
public:
	surfaceInfo_v 	mSlist;
	boltInfo_v		mBltlist;
	boneInfo_v		mBlist;
// save from here
	int				mModelindex;
	qhandle_t		mCustomShader;
	qhandle_t		mCustomSkin;
	int				mModelBoltLink;
	int				mSurfaceRoot;
	int				mLodBias;
	int				mNewOrigin;	// this contains the bolt index of the new origin for this model
#ifdef _G2_GORE
	int				mGoreSetTag;
#endif
	qhandle_t		mModel;		// this and the next entries do NOT go across the network. They are for gameside access ONLY
	char			mFileName[MAX_QPATH];
	int				mAnimFrameDefault;
	int				mSkelFrameNum;
	int				mMeshFrameNum;
	int				mFlags;	// used for determining whether to do full collision detection against this object
// to here
	size_t			*mTransformedVertsArray;	// used to create an array of pointers to transformed verts per surface for collision detection
	CBoneCache		*mBoneCache;
	int				mSkin;

	// these occasionally are not valid (like after a vid_restart)
	// call the questionably efficient G2_SetupModelPointers(this) to insure validity
	bool				mValid; // all the below are proper and valid
	const model_s		*currentModel;
	int					currentModelSize;
	const model_s		*animModel;
	int					currentAnimModelSize;
	const mdxaHeader_t	*aHeader;

#ifdef _G2_LISTEN_SERVER_OPT
	int					entityNum;
#endif

	CGhoul2Info():
	mModelindex(-1),
	mCustomShader(0),
	mCustomSkin(0),
	mModelBoltLink(0),
	mSurfaceRoot(0),
	mLodBias(0),
	mNewOrigin(-1),
#ifdef _G2_GORE
	mGoreSetTag(0),
#endif
	mModel(0),
	mAnimFrameDefault(0),
	mSkelFrameNum(-1),
	mMeshFrameNum(-1),
	mFlags(0),
	mTransformedVertsArray(0),
	mBoneCache(0),
	mSkin(0),
	mValid(false),
	currentModel(0),
	currentModelSize(0),
	animModel(0),
	currentAnimModelSize(0),
	aHeader(0)
#ifdef _G2_LISTEN_SERVER_OPT
	, entityNum(ENTITYNUM_NONE)
#endif
	{
		mFileName[0] = 0;
	}
}; 

class CGhoul2Info_v;

class IGhoul2InfoArray
{
public:
	virtual ~IGhoul2InfoArray() {}

	virtual int New()=0;
	virtual void Delete(int handle)=0;
	virtual bool IsValid(int handle) const=0;
	virtual vector<CGhoul2Info> &Get(int handle)=0;
	virtual const vector<CGhoul2Info> &Get(int handle) const=0;
};

//externing this out of headers for re access :/
IGhoul2InfoArray &_TheGhoul2InfoArray();

extern const CGhoul2Info NullG2;
class CGhoul2Info_v
{
	IGhoul2InfoArray &InfoArray() const
	{
		return _TheGhoul2InfoArray();
	}

	void Alloc()
	{
		assert(!mItem); //already alloced
		mItem=InfoArray().New();
		assert(!Array().size());
	}
	void Free()
	{
		if (mItem)
		{
			assert(InfoArray().IsValid(mItem));
			InfoArray().Delete(mItem);
			mItem=0;
		}
	}
	vector<CGhoul2Info> &Array()
	{
		assert(InfoArray().IsValid(mItem));
		return InfoArray().Get(mItem);
	}
	const vector<CGhoul2Info> &Array() const
	{
		assert(InfoArray().IsValid(mItem));
		return InfoArray().Get(mItem);
	}
public:
	int mItem;	//dont' be bad and muck with this
	CGhoul2Info_v()
	{
		mItem=0;
	}
	CGhoul2Info_v(const int item)
	{	//be VERY carefull with what you pass in here
		mItem=item;
	}
	~CGhoul2Info_v()
	{
		Free(); //this had better be taken care of via the clean ghoul2 models call
	}
	void operator=(const CGhoul2Info_v &other)
	{
		mItem=other.mItem;
	}
	void operator=(const int otherItem)	//assigning one from the VM side item number
	{
		mItem=otherItem;
	}
	void DeepCopy(const CGhoul2Info_v &other)
	{
		Free();
		if (other.mItem)
		{
			Alloc();
			Array()=other.Array();
			int i;
			for (i=0;i<size();i++)
			{
				Array()[i].mBoneCache=0;
				Array()[i].mTransformedVertsArray=0;
				Array()[i].mSkelFrameNum=0;
				Array()[i].mMeshFrameNum=0;
			}
		}
	}
	CGhoul2Info &operator[](int idx)
	{
		if ( IsValid() && idx >= 0 && idx < size() /*&& mItem */ )
			return Array()[idx];
		else
			return const_cast<CGhoul2Info&>( NullG2 );
	}
	const CGhoul2Info &operator[](int idx) const
	{
		if ( idx >=0 && idx < size() /*&& mItem */ )
			return Array()[idx];
		else
			return NullG2;
	}
	void resize(int num)
	{
		assert(num>=0);
		if (num)
		{
			if (!mItem)
			{
				Alloc();
			}
		}
		if (mItem||num)
		{
			Array().resize(num);
		}
	}
	void clear()
	{
		Free();
	}
	void push_back(const CGhoul2Info &model)
	{
		if (!mItem)
		{
			Alloc();
		}
		Array().push_back(model);
	}
	int size() const
	{
		if (!IsValid())
		{
			return 0;
		}
		return Array().size();
	}
	bool IsValid() const
	{
		return InfoArray().IsValid(mItem);
	}
	void kill()
	{
		// this scary method zeros the infovector handle without actually freeing it
		// it is used for some places where a copy is made, but we don't want to go through the trouble
		// of making a deep copy
		mItem=0;
	}
};

// collision detection stuff
#define G2_FRONTFACE 1
#define	G2_BACKFACE	 0


// calling defines for the trace function
enum EG2_Collision
{
	G2_NOCOLLIDE,
	G2_COLLIDE,
	G2_RETURNONHIT
};


//====================================================================
