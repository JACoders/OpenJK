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

#include "../ghoul2/ghoul2_shared.h"
#include "../qcommon/q_shared.h"

#ifdef _G2_GORE

#define MAX_LODS (8)
struct GoreTextureCoordinates
{
	float *tex[MAX_LODS];

	GoreTextureCoordinates();
	~GoreTextureCoordinates();
};

int AllocGoreRecord();
GoreTextureCoordinates *FindGoreRecord(int tag);
void DeleteGoreRecord(int tag);

struct SGoreSurface
{
	int			shader;
	int			mGoreTag;
	int			mDeleteTime;
	int			mFadeTime;
	bool		mFadeRGB;

	int			mGoreGrowStartTime;
	int			mGoreGrowEndTime;    // set this to -1 to disable growing
	//curscale = (curtime-mGoreGrowStartTime)*mGoreGrowFactor + mGoreGrowOffset;
	float		mGoreGrowFactor;
	float		mGoreGrowOffset;
};

class CGoreSet
{
public:
	int		 mMyGoreSetTag;
	unsigned char mRefCount;
	std::multimap<int,SGoreSurface> mGoreRecords; // a map from surface index
	CGoreSet(int tag) : mMyGoreSetTag(tag), mRefCount(0) {}
	~CGoreSet();
};

CGoreSet	*FindGoreSet(int goreSetTag);
CGoreSet	*NewGoreSet();
void		DeleteGoreSet(int goreSetTag);

#endif // _G2_GORE

//rww - RAGDOLL_BEGIN

/// ragdoll stuff
struct SRagDollEffectorCollision
{
	vec3_t			effectorPosition;
	const trace_t	&tr;
	bool			useTracePlane;
	SRagDollEffectorCollision(const vec3_t effectorPos,const trace_t &t) :
		tr(t),
		useTracePlane(false)
	{
		VectorCopy(effectorPos,effectorPosition);
	}
};

class CRagDollUpdateParams
{
public:
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t velocity;
	//CServerEntity *me;
	int	me; //index!
	int settleFrame;

	//at some point I'll want to make VM callbacks in here. For now I am just doing nothing.
	virtual void EffectorCollision(const SRagDollEffectorCollision &data)
	{
	//	assert(0); // you probably meant to override this
	}
	virtual void RagDollBegin()
	{
	//	assert(0); // you probably meant to override this
	}
	virtual void RagDollSettled()
	{
	//	assert(0); // you probably meant to override this
	}

	virtual void Collision()
	{
	//	assert(0); // you probably meant to override this
		// we had a collision, uhh I guess call SetRagDoll RP_DEATH_COLLISION
	}

#ifdef _DEBUG
	virtual void DebugLine(const vec3_t p1,const vec3_t p2,bool bbox) {assert(0);}
#endif
};


class CRagDollParams
{
public:

	enum ERagPhase
	{
		RP_START_DEATH_ANIM,
		RP_END_DEATH_ANIM,
		RP_DEATH_COLLISION,
		RP_CORPSE_SHOT,
		RP_GET_PELVIS_OFFSET,  // this actually does nothing but set the pelvisAnglesOffset, and pelvisPositionOffset
		RP_SET_PELVIS_OFFSET,  // this actually does nothing but set the pelvisAnglesOffset, and pelvisPositionOffset
		RP_DISABLE_EFFECTORS  // this removes effectors given by the effectorsToTurnOff member
	};
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t pelvisAnglesOffset;    // always set on return, an argument for RP_SET_PELVIS_OFFSET
	vec3_t pelvisPositionOffset; // always set on return, an argument for RP_SET_PELVIS_OFFSET

	float fImpactStrength; //should be applicable when RagPhase is RP_DEATH_COLLISION
	float fShotStrength; //should be applicable for setting velocity of corpse on shot (probably only on RP_CORPSE_SHOT)
	//CServerEntity *me;
	int me;

	//rww - we have convenient animation/frame access in the game, so just send this info over from there.
	int startFrame;
	int endFrame;

	int collisionType; // 1 = from a fall, 0 from effectors, this will be going away soon, hence no enum

	qboolean CallRagDollBegin; // a return value, means that we are now begininng ragdoll and the NPC stuff needs to happen

	ERagPhase RagPhase;

// effector control, used for RP_DISABLE_EFFECTORS call

	enum ERagEffector
	{
		RE_MODEL_ROOT=			0x00000001, //"model_root"
		RE_PELVIS=				0x00000002, //"pelvis"
		RE_LOWER_LUMBAR=		0x00000004, //"lower_lumbar"
		RE_UPPER_LUMBAR=		0x00000008, //"upper_lumbar"
		RE_THORACIC=			0x00000010, //"thoracic"
		RE_CRANIUM=				0x00000020, //"cranium"
		RE_RHUMEROUS=			0x00000040, //"rhumerus"
		RE_LHUMEROUS=			0x00000080, //"lhumerus"
		RE_RRADIUS=				0x00000100, //"rradius"
		RE_LRADIUS=				0x00000200, //"lradius"
		RE_RFEMURYZ=			0x00000400, //"rfemurYZ"
		RE_LFEMURYZ=			0x00000800, //"lfemurYZ"
		RE_RTIBIA=				0x00001000, //"rtibia"
		RE_LTIBIA=				0x00002000, //"ltibia"
		RE_RHAND=				0x00004000, //"rhand"
		RE_LHAND=				0x00008000, //"lhand"
		RE_RTARSAL=				0x00010000, //"rtarsal"
		RE_LTARSAL=				0x00020000, //"ltarsal"
		RE_RTALUS=				0x00040000, //"rtalus"
		RE_LTALUS=				0x00080000, //"ltalus"
		RE_RRADIUSX=			0x00100000, //"rradiusX"
		RE_LRADIUSX=			0x00200000, //"lradiusX"
		RE_RFEMURX=				0x00400000, //"rfemurX"
		RE_LFEMURX=				0x00800000, //"lfemurX"
		RE_CEYEBROW=			0x01000000 //"ceyebrow"
	};

	ERagEffector effectorsToTurnOff;  // set this to an | of the above flags for a RP_DISABLE_EFFECTORS

};
//rww - RAGDOLL_END
