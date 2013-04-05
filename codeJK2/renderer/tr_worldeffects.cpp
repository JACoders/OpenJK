// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



//#include "stdafx.h"
//#include "q_math.h"
//#include "QSupport.h"
#include "tr_local.h"
#include "tr_WorldEffects.h"

#pragma warning( disable : 4512 )

inline void VectorMA( vec3_t vecAdd, const float scale, const vec3_t vecScale)
{
	vecAdd[0] += (scale * vecScale[0]);
	vecAdd[1] += (scale * vecScale[1]);
	vecAdd[2] += (scale * vecScale[2]);
}

#define GLS_ALPHA			(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)

static bool debugShowWind = false;
static int	originContents;

extern qboolean ParseVector( const char **text, int count, float *v );
extern void SetViewportAndScissor( void );










float FloatRand(void) 
{ 
//	char	temp[1000];
	float	result;
	float	r;

	r = (float)rand();
	result = r / (float)RAND_MAX;
//	sprintf(temp, "%f - %f\n", r, result);
//	OutputDebugString(temp);

	return result;
}

void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}







CWorldEffect::CWorldEffect(CWorldEffect *owner) :
	mNext(0),
	mSlave(0),
	mOwner(owner),
	mIsSlave(owner ? true : false),
	mEnabled(true)
{
}

CWorldEffect::~CWorldEffect(void)
{
	if (mIsSlave && mNext)
	{
		delete mNext;
		mNext = 0;
	}
	if (mSlave)
	{
		delete mSlave;
		mSlave = 0;
	}
}


bool CWorldEffect::Command(const char  *command)
{
	if (mSlave)
	{
		if (mSlave->Command(command))
		{
			return true;
		}
	}
	if (mIsSlave && mNext)
	{
		if (mNext->Command(command))
		{
			return true;
		}
	}

	return false;
}

void CWorldEffect::ParmUpdate(CWorldEffectsSystem *system, int which)
{ 
	if (mSlave)
	{
		mSlave->ParmUpdate(system, which);
	}
	if (mIsSlave && mNext)
	{
		mNext->ParmUpdate(system, which);
	}
}

void CWorldEffect::ParmUpdate(CWorldEffect *effect, int which) 
{ 
	if (mSlave)
	{
		mSlave->ParmUpdate(effect, which);
	}
	if (mIsSlave && mNext)
	{
		mNext->ParmUpdate(effect, which);
	}
}

void CWorldEffect::SetVariable(int which, bool newValue, bool doSlave)
{
	if (doSlave)
	{
		mSlave->SetVariable(which, newValue, doSlave);
	}
	if (doSlave && mIsSlave && mNext)
	{
		mNext->SetVariable(which, newValue, doSlave);
	}

	switch(which)
	{
		case WORLDEFFECT_ENABLED:
			mEnabled = newValue;
			break;
	}
}

void CWorldEffect::SetVariable(int which, float newValue, bool doSlave)
{
	if (doSlave)
	{
		mSlave->SetVariable(which, newValue, doSlave);
	}
	if (doSlave && mIsSlave && mNext)
	{
		mNext->SetVariable(which, newValue, doSlave);
	}
}

void CWorldEffect::SetVariable(int which, int newValue, bool doSlave)
{
	if (doSlave)
	{
		mSlave->SetVariable(which, newValue, doSlave);
	}
	if (doSlave && mIsSlave && mNext)
	{
		mNext->SetVariable(which, newValue, doSlave);
	}
}

void CWorldEffect::SetVariable(int which, vec3_t newValue, bool doSlave)
{
	if (doSlave)
	{
		mSlave->SetVariable(which, newValue, doSlave);
	}
	if (doSlave && mIsSlave && mNext)
	{
		mNext->SetVariable(which, newValue, doSlave);
	}
}

void CWorldEffect::AddSlave(CWorldEffect *slave)
{
	slave->SetNext(mSlave);
	mSlave = slave;

	slave->SetIsSlave(true);
	slave->SetOwner(this);
}

void CWorldEffect::Update(CWorldEffectsSystem *system, float elapseTime)
{
	if (mSlave && mEnabled)
	{
		mSlave->Update(system, elapseTime);
	}
	if (mIsSlave && mNext)
	{
		mNext->Update(system, elapseTime);
	}
}

void CWorldEffect::Render(CWorldEffectsSystem *system)
{
	if (mSlave && mEnabled)
	{
		mSlave->Render(system);
	}
	if (mIsSlave && mNext)
	{
		mNext->Render(system);
	}
}












CWorldEffectsSystem::CWorldEffectsSystem(void) :
	mList(0),
	mLast(0)
{
}

CWorldEffectsSystem::~CWorldEffectsSystem(void)
{
	CWorldEffect	*next;

	while(mList)
	{
		next = mList->GetNext();
		delete mList;
		mList = next;
	}
}

void CWorldEffectsSystem::AddWorldEffect(CWorldEffect *effect)
{
	if (!mList)
	{
		mList = mLast = effect;
	}
	else
	{
		mLast->SetNext(effect);
		mLast = effect;
	}
}

bool CWorldEffectsSystem::Command(const char *command) 
{ 
	CWorldEffect	*current;

	current = mList;
	while(current)
	{
		if (current->Command(command))
		{
			return true;
		}
		current = current->GetNext();
	}

	return false;
}

void CWorldEffectsSystem::Update(float elapseTime)
{
	CWorldEffect	*current;

	current = mList;
	while(current)
	{
		current->Update(this, elapseTime);
		current = current->GetNext();
	}
}

void CWorldEffectsSystem::ParmUpdate(int which)
{
	CWorldEffect	*current;

	current = mList;
	while(current)
	{
		current->ParmUpdate(this, which);
		current = current->GetNext();
	}
}

void CWorldEffectsSystem::Render(void)
{
	CWorldEffect	*current;

	current = mList;
	while(current)
	{
		current->Render(this);
		current = current->GetNext();
	}
}









class CRainSystem : public CWorldEffectsSystem
{
private:
	// configurable
	int			mMaxRain;
	float		mRainHeight;
	vec3_t		mSpread;
	float		mAlpha;
	float		mWindAngle;

	image_t		*mImage;
	vec3_t		mMinVelocity, mMaxVelocity;
	int			mNextWindGust, mWindDuration, mWindLow;
	float		mWindMin, mWindMax;
	vec3_t		mWindDirection, mNewWindDirection, mWindSpeed;
	int			mWindChange;

	SParticle	*mRainList;
	float		mFadeAlpha;
	bool		mIsRaining;

public:
	enum
	{
		RAINSYSTEM_WIND_DIRECTION,
		RAINSYSTEM_WIND_SPEED,
	};

public:
	CRainSystem(int maxRain);
	~CRainSystem(void);

	virtual	int			GetIntVariable(int which);
	virtual	SParticle	*GetParticleVariable(int which);
	virtual float		GetFloatVariable(int which);
	virtual	float		*GetVecVariable(int which);

	virtual	bool	Command(const char *command);

	virtual	void	Update(float elapseTime);
	virtual	void	Render(void);

			void	Init(void);

			bool	IsRaining() { return mIsRaining; }
};




class CMistyFog : public CWorldEffect
{
private:
//	GLuint		mImage;
//	image_t		*mImage;
	GLfloat		mTextureCoords[2][2];
	GLfloat		mAlpha;
	bool		mAlphaFade, mRendering, mBuddy;
	float		mSpeed, mAlphaDirection;
	float		mCurrentSize, mMinSize, mMaxSize;
	vec3_t		mWindTransform;

	int				mWidth, mHeight;
	unsigned char	*mData;

	const	float	mSize;

public:
	enum
	{
		MISTYFOG_RENDERING = WORLDEFFECT_END
	};

public:
	CMistyFog(int index, CWorldEffect *owner = 0, bool buddy = false);

//			image_t	*GetImage(void) { return mImage; }

			int				GetWidth(void) { return mWidth; }
			int				GetHeight(void) { return mHeight; }
			unsigned char	*GetData(void) { return mData; }
			GLfloat			GetTextureCoord(int s, int y) { return mTextureCoords[s][y]; }
			float			GetAlpha(void) { return mAlpha; }
			bool			GetRendering(void) { return mRendering; }

	virtual	void	Update(CWorldEffectsSystem *system, float elapseTime);
	virtual	void	ParmUpdate(CWorldEffectsSystem *system, int which);
	virtual	void	ParmUpdate(CWorldEffect *effect, int which);
	virtual	void	Render(CWorldEffectsSystem *system);

			void	CreateTextureCoords(void);
};

CMistyFog::CMistyFog(int index, CWorldEffect *owner, bool buddy) :
	CWorldEffect(owner),
		
	mSize(0.05f*2.0f),
	mMinSize(0.05f*3.0f),
	mMaxSize(0.15f*2.0f),
	mAlpha(1.0f),
	mAlphaFade(false),
	mBuddy(buddy)
{
	char			name[1024];
	unsigned char	*pos;
	int				x, y;

	if (mBuddy)
	{
		mRendering = false;

//		mImage = ((CMistyFog *)owner)->GetImage();
		mData = ((CMistyFog *)owner)->GetData();
		mWidth = ((CMistyFog *)owner)->GetWidth();
		mHeight = ((CMistyFog *)owner)->GetHeight();
	}
	else
	{
		sprintf(name, "gfx/world/fog%d.tga", index);

		R_LoadImage( name, &mData, &mWidth, &mHeight );
		if (!mData)
		{
			ri.Error (ERR_DROP, "Could not load %s", name);
		}

		pos = mData;
		for(y=0;y<mHeight;y++)
		{
			for(x=0;x<mWidth;x++)
			{
				pos[3] = pos[0];
				pos += 4;
			}
		}

//		mImage = R_CreateImage(name, mData, mWidth, mHeight, false, true, false, GL_REPEAT);

		mRendering = true;
		AddSlave(new CMistyFog(index, this, true));
	}

	mSpeed = 90.0 + FloatRand() * 20.0;

	CreateTextureCoords();
}

void CMistyFog::Update(CWorldEffectsSystem *system, float elapseTime)
{
	bool	removeImage = false;
	float	forwardWind, rightWind;

	CWorldEffect::Update(system, elapseTime);

	if (!mRendering)
	{
		return;
	}

	// translate

	forwardWind = DotProduct(mWindTransform, backEnd.viewParms.or.axis[0]);
	rightWind = DotProduct(mWindTransform, backEnd.viewParms.or.axis[1]);

	mTextureCoords[0][0] += rightWind / mSpeed;
	mTextureCoords[1][0] += rightWind / mSpeed;

	mTextureCoords[0][0] -= forwardWind / mSpeed / 4.0;
	mTextureCoords[0][1] -= forwardWind / mSpeed / 4.0;
	mTextureCoords[1][0] += forwardWind / mSpeed / 4.0;
	mTextureCoords[1][1] += forwardWind / mSpeed / 4.0;

/*	if (mTextureCoords[0][0] > mTextureCoords[1][0] ||
		mTextureCoords[0][1] > mTextureCoords[1][1])
	{

		mAlphaFade = true;
		mAlphaDirection = -1.0;
		mAlpha = -1.0;
	}
*/
	if ((fabs(mTextureCoords[0][0] - mTextureCoords[1][0]) < mMinSize ||
		fabs(mTextureCoords[0][1] - mTextureCoords[1][1]) < mMinSize))// && forwardWind > 0.0)
	{
		removeImage = true;
	}

	if ((fabs(mTextureCoords[0][0] - mTextureCoords[1][0]) > mMaxSize ||
		fabs(mTextureCoords[0][1] - mTextureCoords[1][1]) > mMaxSize))// && forwardWind < 0.0)
	{
		removeImage = true;
	}

	if (mTextureCoords[0][0] < mCurrentSize || mTextureCoords[0][1] < mCurrentSize ||
		mTextureCoords[0][0] > 1.0-mCurrentSize || mTextureCoords[0][1] > 1.0-mCurrentSize)
	{
//		mAlphaFade = true;
	}
	if (mTextureCoords[1][0] < mCurrentSize || mTextureCoords[1][1] < mCurrentSize ||
		mTextureCoords[1][0] > 1.0-mCurrentSize || mTextureCoords[1][1] > 1.0-mCurrentSize)
	{
//		mAlphaFade = true;
	}

	if (removeImage && !mAlphaFade)
	{
		mAlphaFade = true;
		mAlphaDirection = -0.025f;
		if (mBuddy)
		{
			mOwner->ParmUpdate(this, MISTYFOG_RENDERING);
		}
		else if (mSlave)
		{
			mSlave->ParmUpdate(this, MISTYFOG_RENDERING);
		}
	}

	if (mAlphaFade)
	{
		mAlpha += mAlphaDirection * 0.4;
		if (mAlpha < 0.0)
		{
			mRendering = false;
			mAlpha = 0.0;
		}
		else if (mAlpha >= 1.0)
		{
			mAlphaFade = false;
			mAlpha = 1.0;
		}
	}
}

void CMistyFog::ParmUpdate(CWorldEffectsSystem *system, int which)
{
	CWorldEffect::ParmUpdate(system, which);

	switch(which)
	{
		case CRainSystem::RAINSYSTEM_WIND_DIRECTION:
			VectorCopy(system->GetVecVariable(which), mWindTransform);
			break;
	}
}

void CMistyFog::ParmUpdate(CWorldEffect *effect, int which)
{
	CWorldEffect::ParmUpdate(effect, which);

	switch(which)
	{
		case MISTYFOG_RENDERING:
			if (effect == mOwner || effect == mSlave)
			{
				mAlpha = 0.0;
				mAlphaDirection = 0.025f;
				mAlphaFade = true;
				CreateTextureCoords();
				mRendering = true;
			}
			break;
	}
}

void CMistyFog::Render(CWorldEffectsSystem *system) 
{ 
	CWorldEffect::Render(system);

/*	if (!mRendering)
	{
		return;
	}

	GL_Bind(mImage);
	GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE);

//	qglColor4f(1.0, 1.0, 1.0, mAlpha*0.4);

	if (mSlave)
	{
		qglColor4f(1.0, 0.0, 0.0, mAlpha);	
	}
	else
	{
		qglColor4f(0.0, 1.0, 0.0, mAlpha);	
	}

	qglBegin(GL_QUADS);
	qglTexCoord2f(mTextureCoords[0][0], mTextureCoords[0][1]);
	qglVertex3f(-10, 10, -10);

	qglTexCoord2f(mTextureCoords[1][0], mTextureCoords[0][1]);
	qglVertex3f(10, 10, -10);

	qglTexCoord2f(mTextureCoords[1][0], mTextureCoords[1][1]);
	qglVertex3f(10, -10, -10);

	qglTexCoord2f(mTextureCoords[0][0], mTextureCoords[1][1]);
	qglVertex3f(-10, -10, -10);

	qglEnd();*/
}

void CMistyFog::CreateTextureCoords(void)
{
	float	xStart, yStart;
	float	forwardWind, rightWind;

	mSpeed = 800.0 + FloatRand() * 2000.0;
	mSpeed /= 4.0;

	forwardWind = DotProduct(mWindTransform, backEnd.viewParms.or.axis[0]);
	rightWind = DotProduct(mWindTransform, backEnd.viewParms.or.axis[1]);

	if (forwardWind > 0.5)
	{	// moving away, so make the size smaller
		mCurrentSize = mMinSize + (FloatRand() * mMinSize * 0.01);
//		mCurrentSize = mMinSize / 3.0;
	}
	else if (forwardWind < -0.5)
	{	// moving towards, so make bigger
//		mCurrentSize = (mSize * 0.8) + (FloatRand() * mSize * 0.8);
		mCurrentSize = mMaxSize - (FloatRand() * mMinSize);
	}
	else
	{	// normal range
		mCurrentSize = mMinSize * 1.5 + (FloatRand() * mSize);
	}

	mCurrentSize /= 2.0;

	xStart = (1.0 - mCurrentSize - 0.40) * FloatRand() + 0.20;
	yStart = (1.0 - mCurrentSize - 0.40) * FloatRand() + 0.20;

	mTextureCoords[0][0] = xStart - mCurrentSize;
	mTextureCoords[0][1] = yStart - mCurrentSize;
	mTextureCoords[1][0] = xStart + mCurrentSize;
	mTextureCoords[1][1] = yStart + mCurrentSize;
}









#define	MISTYFOG_WIDTH	30
#define MISTYFOG_HEIGHT	30


class CMistyFog2 : public CWorldEffect
{
protected:
	vec4_t			mColors[MISTYFOG_HEIGHT][MISTYFOG_WIDTH];
	vec3_t			mVerts[MISTYFOG_HEIGHT][MISTYFOG_WIDTH];
	unsigned int	mIndexes[MISTYFOG_HEIGHT-1][MISTYFOG_WIDTH-1][4];
	float			mAlpha;

	float			mFadeAlpha;

public:
	CMistyFog2(void);

	virtual	bool	Command(const char  *command);

			void	UpdateTexture(CMistyFog *fog);

	virtual	void	Update(CWorldEffectsSystem *system, float elapseTime);
	virtual	void	Render(CWorldEffectsSystem *system);
};


CMistyFog2::CMistyFog2(void) :
	CWorldEffect(),
	mAlpha(0.3f),

	mFadeAlpha(0.0f)
{
	int			x, y;
	float		xStep, yStep;

	AddSlave(new CMistyFog(2));
	AddSlave(new CMistyFog(2));

	xStep = 20.0f / (MISTYFOG_WIDTH-1);
	yStep = 20.0f / (MISTYFOG_HEIGHT-1);

	for(y=0;y<MISTYFOG_HEIGHT;y++)
	{
		for(x=0;x<MISTYFOG_WIDTH;x++)
		{
			mVerts[y][x][0] = -10 + (x * xStep) + ri.flrand(-xStep / 16.0, xStep / 16.0);
			mVerts[y][x][1] = 10 - (y * yStep) + ri.flrand(-xStep / 16.0, xStep / 16.0);
			mVerts[y][x][2] = -10;

			mColors[y][x][0] = 1.0;
			mColors[y][x][1] = 1.0;
			mColors[y][x][2] = 1.0;

			if (y < MISTYFOG_HEIGHT-1 && x < MISTYFOG_WIDTH-1)
			{
				mIndexes[y][x][0] = (y*MISTYFOG_WIDTH) + x;
				mIndexes[y][x][1] = (y*MISTYFOG_WIDTH) + x+1;
				mIndexes[y][x][2] = ((y+1)*MISTYFOG_WIDTH) + x+1;
				mIndexes[y][x][3] = ((y+1)*MISTYFOG_WIDTH) + x;
			}
		}
	}
}

bool CMistyFog2::Command(const char  *command)
{
	char	*token;

	if (CWorldEffect::Command(command))
	{
		return true;
	}

	token = COM_ParseExt(&command, false);
	if (strcmpi(token, "fog") != 0)
	{
		return false;
	}

	token = COM_ParseExt(&command, false);
	if (strcmpi(token, "density") == 0)
	{
		token = COM_ParseExt(&command, false);
		mAlpha = atof(token);

		return true;
	}

	return false;
}

void CMistyFog2::Update(CWorldEffectsSystem *system, float elapseTime)
{
	CMistyFog	*current;
	int			x, y;

	if (originContents & CONTENTS_OUTSIDE && !(originContents & CONTENTS_WATER))
	{
		if (mFadeAlpha < 1.0)
		{
			mFadeAlpha += elapseTime / 2.0;
		}
		if (mFadeAlpha > 1.0)
		{
			mFadeAlpha = 1.0;
		}
	}
	else
	{
		if (mFadeAlpha > 0.0)
		{
			mFadeAlpha -= elapseTime / 2.0;
		}

		if (mFadeAlpha <= 0.0)
		{
			return;
		}
	}

	for(y=0;y<MISTYFOG_HEIGHT;y++)
	{
		for(x=0;x<MISTYFOG_WIDTH;x++)
		{
			mColors[y][x][3] = 0.0;
		}
	}

	CWorldEffect::Update(system, elapseTime);

	current = (CMistyFog *)mSlave;
	while(current)
	{
		UpdateTexture(current);
		UpdateTexture((CMistyFog *)current->GetSlave());
		current = (CMistyFog *)current->GetNext();
	}
}

void CMistyFog2::UpdateTexture(CMistyFog *fog)
{
	int				x, y, tx, ty;
	float			xSize, ySize;
	float			xStep, yStep;
	float			xPos, yPos;
	unsigned char	*data = fog->GetData();
	int				width = fog->GetWidth();
	int				height = fog->GetHeight();
	int				andWidth, andHeight;
	float			alpha = fog->GetAlpha() * mAlpha * (1.0/255.0) * mFadeAlpha;
	float			*color;

	if (!fog->GetRendering())
	{
		return;
	}

	andWidth = width-1;		// width must be power of 2
	andHeight = height-1;	// height must be power of 2
	xSize = fog->GetTextureCoord(1, 0) - fog->GetTextureCoord(0, 0);
	ySize = fog->GetTextureCoord(1, 1) - fog->GetTextureCoord(0, 1);
	xStep = xSize / (float)MISTYFOG_WIDTH;
	yStep = ySize / (float)MISTYFOG_HEIGHT;

	color = &mColors[0][0][3];
	for(y=0,yPos = fog->GetTextureCoord(0, 1);y<MISTYFOG_HEIGHT;y++, yPos += yStep)
	{
		for(x=0,xPos = fog->GetTextureCoord(0, 0);x<MISTYFOG_WIDTH;x++, xPos += xStep)
		{
			tx = xPos * width;
			tx &= andWidth;
			ty = yPos * height;
			ty &= andHeight;

			(*color) += data[(ty*width + tx)* 4] * alpha;
			color += 4;
		}
	}
}

void CMistyFog2::Render(CWorldEffectsSystem *system)
{
	if (mFadeAlpha <= 0.0)
	{
		return;
	}

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
    qglLoadIdentity ();
	MYgluPerspective (80.0,  1.0,  4,  2048.0);

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
    qglLoadIdentity ();
    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up
    qglRotatef (0,  1, 0, 0);
    qglRotatef (-90,  0, 1, 0);
    qglRotatef (-90,  0, 0, 1);

	qglDisable(GL_TEXTURE_2D);
	GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE);
	qglShadeModel (GL_SMOOTH);

	qglColorPointer(4, GL_FLOAT, 0, mColors);
	qglEnableClientState(GL_COLOR_ARRAY);

	qglVertexPointer( 3, GL_FLOAT, 0, mVerts );
	qglEnableClientState(GL_VERTEX_ARRAY);

	if (qglLockArraysEXT) 
	{
		qglLockArraysEXT(0, MISTYFOG_HEIGHT*MISTYFOG_WIDTH);
	}
	qglDrawElements(GL_QUADS, (MISTYFOG_HEIGHT-1)*(MISTYFOG_WIDTH-1)*4, GL_UNSIGNED_INT, mIndexes);
	if ( qglUnlockArraysEXT ) 
	{
		qglUnlockArraysEXT();
	}

	qglDisableClientState(GL_COLOR_ARRAY);
//	qglDisableClientState(GL_VERTEX_ARRAY);	 backend doesn't ever re=enable this properly

	qglPopMatrix();
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);	// bug somewhere in the backend which requires this
}

















































class CWind : public CWorldEffect
{
private:
	vec4_t	mPlanes[3];		// x y z normal, distance
	float	mMaxDistance[3];
	vec3_t	mVelocity;
	int		mNumPlanes;
	int		mAffectedDuration;
	int		*mAffectedCount;
	vec3_t	mPoint, mSize;
	bool	mGlobal;

public:
	CWind(bool global = false);
	CWind(vec3_t point, vec3_t velocity, vec3_t size, int duration, bool global = false);
	~CWind(void);

	virtual	void	Update(CWorldEffectsSystem *system, float elapseTime);
	virtual	void	ParmUpdate(CWorldEffectsSystem *system, int which);
	virtual	void	Render(CWorldEffectsSystem *system);

			void	UpdateParms(vec3_t point, vec3_t velocity, vec3_t size, int duration);
};








CWind::CWind(bool global) :
	CWorldEffect(),
	mNumPlanes(0),
	mAffectedCount(0),
	mGlobal(global)
{
	mEnabled = false;
}

CWind::CWind(vec3_t point, vec3_t velocity, vec3_t size, int duration, bool global) :
	CWorldEffect(),
	mNumPlanes(0),
	mAffectedCount(0),
	mGlobal(global)
{
	UpdateParms(point, velocity, size, duration);
}

CWind::~CWind(void)
{
	if (mAffectedCount)
	{
		delete [] mAffectedCount;
		mAffectedCount = 0;
	}
}

void CWind::UpdateParms(vec3_t point, vec3_t velocity, vec3_t size, int duration)
{
	vec3_t	normalDistance;

	mNumPlanes = 0;

	VectorCopy(point, mPoint);
	VectorCopy(size, mSize);
	mSize[0] /= 2.0;
	VectorScale(mSize, 2, mSize);
	VectorCopy(velocity, mVelocity);

	VectorCopy(velocity, mPlanes[mNumPlanes]);
	VectorNormalize(mPlanes[mNumPlanes]);
	mPlanes[mNumPlanes][3] = DotProduct(mPoint, mPlanes[mNumPlanes]);
	mMaxDistance[mNumPlanes] = mSize[0];
	mNumPlanes++;

	VectorScale(mPlanes[0], mPlanes[0][3], normalDistance);
	VectorSubtract(mPoint, normalDistance, mPlanes[mNumPlanes]);
	VectorNormalize(mPlanes[mNumPlanes]);
	mPlanes[mNumPlanes][3] = DotProduct(mPoint, mPlanes[mNumPlanes]);
	mMaxDistance[mNumPlanes] = mSize[1];
	mNumPlanes++;

	CrossProduct(mPlanes[0], mPlanes[1], mPlanes[mNumPlanes]);
	VectorNormalize(mPlanes[mNumPlanes]);
	mPlanes[mNumPlanes][3] = DotProduct(mPoint, mPlanes[mNumPlanes]);
	mMaxDistance[mNumPlanes] = mSize[2];
	mNumPlanes++;

	mPlanes[0][3] -= (mSize[0] / 2.0);
	mPlanes[1][3] -= (mSize[1] / 2.0);
	mPlanes[2][3] -= (mSize[2] / 2.0);

	mAffectedDuration = duration;
}

void CWind::Update(CWorldEffectsSystem *system, float elapseTime)
{
	SParticle				*item;
	int						i, j, *affected;
	float					dist, calcDist[3];
	vec3_t					difference;

	if (!mEnabled)
	{
		return;
	}

	VectorSubtract(backEnd.viewParms.or.origin, mPoint, difference);
	if (VectorLength(difference) > 300.0)
	{
		return;
	}

	calcDist[0] = 0.0;
	item = system->GetParticleVariable(WORLDEFFECT_PARTICLES);
	affected = mAffectedCount;
	for(i=system->GetIntVariable(WORLDEFFECT_PARTICLE_COUNT); i; i--)
	{
		if ((*affected))
		{
			(*affected)--;
		}
		else
		{
			if (!mGlobal)
			{
				for(j=0;j<mNumPlanes;j++)
				{
					dist = DotProduct(item->pos, mPlanes[j]) - mPlanes[j][3];

					if (dist < 0.01 || dist > mMaxDistance[j])
					{
						break;
					}
					else
					{
						calcDist[j] = dist;
					}
				}
				if (j != mNumPlanes)
				{
					continue;
				}
			}

			float	scaleLength = 1.0 - (calcDist[0] / mMaxDistance[0]);

			(*affected) = mAffectedDuration * scaleLength;

			VectorMA(item->velocity, elapseTime, mVelocity);
//			VectorMA(item->velocity, scaleLength, mVelocity, item->velocity);
		}
		affected++;
		item++;
	}
}

void CWind::ParmUpdate(CWorldEffectsSystem *system, int which)
{
	CWorldEffect::ParmUpdate(system, which);

	switch(which)
	{
		case WORLDEFFECT_PARTICLE_COUNT:
			if (mAffectedCount)
			{
				delete [] mAffectedCount;
			}
			mAffectedCount = new int[system->GetIntVariable(WORLDEFFECT_PARTICLE_COUNT)];
			memset(mAffectedCount, 0, system->GetIntVariable(WORLDEFFECT_PARTICLE_COUNT)*sizeof(int));
			break;
	}
}

void CWind::Render(CWorldEffectsSystem *system)
{
	vec3_t	output;

	if (!mEnabled || !debugShowWind)
	{
		return;
	}

	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_CULL_FACE);
	GL_State(GLS_ALPHA);

	qglColor4f(1.0, 0.0, 0.0, 0.5);
	qglBegin(GL_QUADS);
	
	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, (mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, (mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, (mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, (mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	
	
	qglColor4f(0.0, 1.0, 0.0, 0.5);
	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	VectorMA(mPoint, (mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	VectorMA(mPoint, (mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, (mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);

	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	VectorMA(output, (mSize[2]/2.0), mPlanes[2], output);
	qglVertex3fv(output);
	

	qglColor4f(0.0, 0.0, 1.0, 0.5);
	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	qglVertex3fv(output);

	VectorMA(mPoint, (mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	VectorMA(output, -(mSize[1]/2.0), mPlanes[1], output);
	qglVertex3fv(output);

	VectorMA(mPoint, (mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	VectorMA(output, (mSize[1]/2.0), mPlanes[1], output);
	qglVertex3fv(output);

	VectorMA(mPoint, -(mSize[0]/2.0), mPlanes[0], output);
	VectorMA(output, -(mSize[2]/2.0), mPlanes[2], output);
	VectorMA(output, (mSize[1]/2.0), mPlanes[1], output);
	qglVertex3fv(output);
	
	
	qglEnd();

	qglEnable(GL_CULL_FACE);
	qglEnable(GL_TEXTURE_2D);
}












#define CONTENTS_X_SIZE		16
#define CONTENTS_Y_SIZE		16
#define CONTENTS_Z_SIZE		8


class CSnowSystem : public CWorldEffectsSystem
{
private:
	// configurable
	float		mAlpha;
	vec3_t		mMinSpread, mMaxSpread;
	vec3_t		mMinVelocity, mMaxVelocity;
	int			mMaxSnowflakes;
	float		mWindDuration, mWindLow;
	float		mWindMin, mWindMax;
	vec3_t		mWindSize;

	image_t		*mImage;
	vec3_t		mMins, mMaxs;
	float		mNextWindGust, mWindLowSize;
	CWind		*mWindGust;

	vec3_t		mWindDirection, mWindSpeed;
	int			mWindChange;

	SParticle	*mSnowList;
	int			mContents[CONTENTS_Z_SIZE][CONTENTS_Y_SIZE][CONTENTS_X_SIZE];
	vec3_t		mContentsSize;
	vec3_t		mContentsStart;

	int			mUpdateCount;
	int			mOverallContents;
	bool		mIsSnowing;

	const		float	mVelocityStabilize;
	const		int		mUpdateMax;

public:
	CSnowSystem(int maxSnowflakes);
	~CSnowSystem(void);

	virtual	int			GetIntVariable(int which);
	virtual	SParticle	*GetParticleVariable(int which);
	virtual	float		*GetVecVariable(int which);

	virtual bool	Command(const char *command);

	virtual	void	Update(float elapseTime);
	virtual	void	Render(void);

			void	Init(void);

			bool	IsSnowing() { return mIsSnowing; }
};

CSnowSystem::CSnowSystem(int maxSnowflakes) :
	mMaxSnowflakes(maxSnowflakes),
	mNextWindGust(0.0),
	mWindLowSize(0.0),
	mWindGust(0),
	mWindChange(0),

	mAlpha(0.4f),
	mWindDuration(2.0f),
	mWindLow(3.0f),
	mWindMin(30.0f), // .6 3
	mWindMax(70.0f),
	mUpdateCount(0),
	mOverallContents(0),

	mVelocityStabilize(18),
	mUpdateMax(10),
	mIsSnowing(false)
{
	mMinSpread[0] = -600;
	mMinSpread[1] = -600;
	mMinSpread[2] = -200;
	mMaxSpread[0] = 600;
	mMaxSpread[1] = 600;
	mMaxSpread[2] = 250;

	mMinVelocity[0] = -15.0;
	mMaxVelocity[0] = 15.0;
	mMinVelocity[1] = -15.0;
	mMaxVelocity[1] = 15.0;
	mMinVelocity[2] = -20.0;
	mMaxVelocity[2] = -70.0;

	mWindSize[0] = 1000.0;
	mWindSize[1] = 300.0;
	mWindSize[2] = 300.0;

	mSnowList = new SParticle[mMaxSnowflakes];

	mContentsSize[0] = (mMaxSpread[0] - mMinSpread[0]) / CONTENTS_X_SIZE;
	mContentsSize[1] = (mMaxSpread[1] - mMinSpread[1]) / CONTENTS_Y_SIZE;
	mContentsSize[2] = (mMaxSpread[2] - mMinSpread[2]) / CONTENTS_Z_SIZE;

	Init();

	AddWorldEffect(mWindGust= new CWind(true));
	ParmUpdate(CWorldEffect::WORLDEFFECT_PARTICLE_COUNT);
}

CSnowSystem::~CSnowSystem(void)
{
	delete [] mSnowList;
}

void CSnowSystem::Init(void)
{
	int			i;
	SParticle	*item;

	mMins[0] = mMaxs[0] = mMins[1] = mMaxs[1] = mMins[2] = mMaxs[2] = 99999;
	item = mSnowList;
	for(i=mMaxSnowflakes;i;i--)
	{
		item->pos[0] = item->pos[1] = item->pos[2] = 99999;
		item->velocity[0] = item->velocity[1] = item->velocity[2] = 0.0;
		item->flags = 0;
		item++;
	}
}

int CSnowSystem::GetIntVariable(int which)
{
	switch(which)
	{
		case CWorldEffect::WORLDEFFECT_PARTICLE_COUNT:
			return mMaxSnowflakes;
	}

	return CWorldEffectsSystem::GetIntVariable(which);
}

SParticle *CSnowSystem::GetParticleVariable(int which)
{
	switch(which)
	{
		case CWorldEffect::WORLDEFFECT_PARTICLES:
			return mSnowList;
	}

	return CWorldEffectsSystem::GetParticleVariable(which);
}

float *CSnowSystem::GetVecVariable(int which) 
{ 
	switch(which)
	{
		case CRainSystem::RAINSYSTEM_WIND_DIRECTION:
			return mWindDirection;
	}
	return 0; 
}

bool CSnowSystem::Command(const char *command)
{
	char	*token;

	if (CWorldEffectsSystem::Command(command))
	{
		return true;
	}

	token = COM_ParseExt(&command, false);

	if (strcmpi(token, "wind") == 0)
	{	// snow wind ( windOriginX windOriginY windOriginZ ) ( windVelocityX windVelocityY windVelocityZ ) ( sizeX sizeY sizeZ )
		vec3_t	origin, velocity, size;

		ParseVector(&command, 3, origin);
		ParseVector(&command, 3, velocity);
		ParseVector(&command, 3, size);

		AddWorldEffect(new CWind(origin, velocity, size, 0));

		return true;
	}
	else if (strcmpi(token, "fog") == 0)
	{	// snow fog
		AddWorldEffect(new CMistyFog2);
		mWindChange = 0;
		return true;
	}
	else if (strcmpi(token, "alpha") == 0)
	{	// snow alpha <float>											default: 0.09
		token = COM_ParseExt(&command, false);
		mAlpha = atof(token);
		return true;
	}
	else if (strcmpi(token, "spread") == 0)
	{	// snow spread ( minX minY minZ ) ( maxX maxY maxZ )			default: ( -600 -600 -200 ) ( 600 600 250 )
		ParseVector(&command, 3, mMinSpread);
		ParseVector(&command, 3, mMaxSpread);
		return true;
	}
	else if (strcmpi(token, "velocity") == 0)
	{	// snow velocity ( minX minY minZ ) ( maxX maxY maxZ )			default: ( -15 -15 -20 ) ( 15 15 -70 )
		ParseVector(&command, 3, mMinSpread);
		ParseVector(&command, 3, mMaxSpread);
		return true;
	}
	else if (strcmpi(token, "blowing") == 0)
	{	
		token = COM_ParseExt(&command, false);
		if (strcmpi(token, "duration") == 0)
		{	// snow blowing duration <int>									default: 2
			token = COM_ParseExt(&command, false);
			mWindDuration = atol(token);
			return true;
		}
		else if (strcmpi(token, "low") == 0)
		{	// snow blowing low <int>										default: 3
			token = COM_ParseExt(&command, false);
			mWindLow = atol(token);
			return true;
		}
		else if (strcmpi(token, "velocity") == 0)
		{	// snow blowing velocity ( min max )							default: ( 30 70 )
			float	data[2];

			ParseVector(&command, 2, data);
			mWindMin = data[0];
			mWindMax = data[1];
			return true;
		}
		else if (strcmpi(token, "size") == 0)
		{	// snow blowing size ( minX minY minZ )							default: ( 1000 300 300 )
			ParseVector(&command, 3, mWindSize);
			return true;
		}
	}

	return false;
}

void CSnowSystem::Update(float elapseTime)
{
	int			i;
	SParticle	*item;
	vec3_t		origin, newMins, newMaxs;
	vec3_t		difference, start;
	bool		resetFlake;
	int			x, y, z;
	int			contents;

	mWindChange--;
	if (mWindChange < 0)
	{
		mWindDirection[0] = 1.0 - (FloatRand() * 2.0);
		mWindDirection[1] = 1.0 - (FloatRand() * 2.0);
		mWindDirection[2] = 0.0;
		VectorNormalize(mWindDirection);
		VectorScale(mWindDirection, 0.025f, mWindSpeed);

		mWindChange = 200 + rand() % 250;
//		mWindChange = 10;

		ParmUpdate(CRainSystem::RAINSYSTEM_WIND_DIRECTION);
	}

	if ((mOverallContents & CONTENTS_OUTSIDE))
	{
		CWorldEffectsSystem::Update(elapseTime);
	}

	VectorCopy(backEnd.viewParms.or.origin, origin);

	mNextWindGust -= elapseTime;
	if (mNextWindGust < 0.0)
	{
		mWindGust->SetVariable(CWorldEffect::WORLDEFFECT_ENABLED, false);
	}

	if (mNextWindGust < mWindLowSize)
	{
		vec3_t		windPos;
		vec3_t		windDirection;

		windDirection[0] = ri.flrand(-1.0, 1.0);
		windDirection[1] = ri.flrand(-1.0, 1.0);
		windDirection[2] = 0.0;//ri.flrand(-0.1, 0.1);
		VectorNormalize(windDirection);
		VectorScale(windDirection, ri.flrand(mWindMin, mWindMax), windDirection);

		VectorCopy(origin, windPos);

		mWindGust->SetVariable(CWorldEffect::WORLDEFFECT_ENABLED, true);
		mWindGust->UpdateParms(windPos, windDirection, mWindSize, 0);

		mNextWindGust = ri.flrand(mWindDuration, mWindDuration*2.0);
		mWindLowSize = -ri.flrand(mWindLow, mWindLow*3.0);
	}

	newMins[0] = mMinSpread[0] + origin[0];
	newMaxs[0] = mMaxSpread[0] + origin[0];

	newMins[1] = mMinSpread[1] + origin[1];
	newMaxs[1] = mMaxSpread[1] + origin[1];

	newMins[2] = mMinSpread[2] + origin[2];
	newMaxs[2] = mMaxSpread[2] + origin[2];

	for(i=0;i<3;i++)
	{
		difference[i] = newMaxs[i] - mMaxs[i];
		if (difference[i] >= 0.0)
		{
			if (difference[i] > newMaxs[i]-newMins[i])
			{
				difference[i] = newMaxs[i]-newMins[i];
			}
			start[i] = newMaxs[i] - difference[i];
		}
		else
		{ 
			if (difference[i] < newMins[i]-newMaxs[i])
			{
				difference[i] = newMins[i]-newMaxs[i];
			}
			start[i] = newMins[i] - difference[i];
		}
	}

//	contentsStart[0] = (((origin[0] + mMinSpread[0]) / mContentsSize[0])) * mContentsSize[0];
//	contentsStart[1] = (((origin[1] + mMinSpread[1]) / mContentsSize[1])) * mContentsSize[1];
//	contentsStart[2] = (((origin[2] + mMinSpread[2]) / mContentsSize[2])) * mContentsSize[2];

	if (fabs(difference[0]) > 25.0 || fabs(difference[1]) > 25.0 || fabs(difference[2]) > 25.0)
	{
		vec3_t		pos;
		int			*store;

		mContentsStart[0] = ((int)((origin[0] + mMinSpread[0]) / mContentsSize[0])) * mContentsSize[0];
		mContentsStart[1] = ((int)((origin[1] + mMinSpread[1]) / mContentsSize[1])) * mContentsSize[1];
		mContentsStart[2] = ((int)((origin[2] + mMinSpread[2]) / mContentsSize[2])) * mContentsSize[2];

		mOverallContents = 0;
		store = (int *)mContents;
		for(z=0,pos[2]=mContentsStart[2];z<CONTENTS_Z_SIZE;z++,pos[2]+=mContentsSize[2])
		{
			for(y=0,pos[1]=mContentsStart[1];y<CONTENTS_Y_SIZE;y++,pos[1]+=mContentsSize[1])
			{
				for(x=0,pos[0]=mContentsStart[0];x<CONTENTS_X_SIZE;x++,pos[0]+=mContentsSize[0])
				{
					contents = ri.CM_PointContents(pos, 0);
					mOverallContents |= contents;
					*store++ = contents;
				}
			}
		}

		item = mSnowList;
		for(i=mMaxSnowflakes;i;i--)
		{
			resetFlake = false;

			if (item->pos[0] < newMins[0] || item->pos[0] > newMaxs[0])
			{
				item->pos[0] = ri.flrand(0.0, difference[0]) + start[0];
				resetFlake = true;
			}
			if (item->pos[1] < newMins[1] || item->pos[1] > newMaxs[1])
			{
				item->pos[1] = ri.flrand(0.0, difference[1]) + start[1];
				resetFlake = true;
			}
			if (item->pos[2] < newMins[2] || item->pos[2] > newMaxs[2])
			{
				item->pos[2] = ri.flrand(0.0, difference[2]) + start[2];
				resetFlake = true;
			}

			if (resetFlake)
			{
				item->velocity[0] = 0.0;
				item->velocity[1] = 0.0;
				item->velocity[2] = ri.flrand(mMaxVelocity[2], mMinVelocity[2]);
			}
			item++;
		}

		VectorCopy(newMins, mMins);
		VectorCopy(newMaxs, mMaxs);
	}

	if (!(mOverallContents & CONTENTS_OUTSIDE))
	{
		mIsSnowing = false;
		return;
	}

	mIsSnowing = true;

	mUpdateCount = (mUpdateCount + 1) % mUpdateMax;

	x = y = z = 0;
	item = mSnowList;
	for(i=mMaxSnowflakes;i;i--)
	{
		resetFlake = false;

//		if ((i & mUpdateCount) == 0)   wrong check
		{
			if (item->velocity[0] < mMinVelocity[0])
			{
				item->velocity[0] += mVelocityStabilize * elapseTime;
			}
			else if (item->velocity[0] > mMaxVelocity[0])
			{
				item->velocity[0] -= mVelocityStabilize * elapseTime;
			}
			else
			{
				item->velocity[0] += ri.flrand(-1.4f, 1.4f);
			}
			if (item->velocity[1] < mMinVelocity[1])
			{
				item->velocity[1] += mVelocityStabilize * elapseTime;
			}
			else if (item->velocity[1] > mMaxVelocity[1])
			{
				item->velocity[1] -= mVelocityStabilize * elapseTime;
			}
			else
			{
				item->velocity[1] += ri.flrand(-1.4f, 1.4f);
			}
			if (item->velocity[2] > mMinVelocity[2])
			{
				item->velocity[2] -= mVelocityStabilize*2.0;
			}
		}
		VectorMA(item->pos, elapseTime, item->velocity);

		if (item->pos[2] < newMins[2])
		{
			resetFlake = true;
		}
		else
		{
//			if ((i & mUpdateCount) == 0)
			{
				x = (item->pos[0] - mContentsStart[0]) / mContentsSize[0];
				y = (item->pos[1] - mContentsStart[1]) / mContentsSize[1];
				z = (item->pos[2] - mContentsStart[2]) / mContentsSize[2];
				if (x < 0 || x >= CONTENTS_X_SIZE ||
					y < 0 || y >= CONTENTS_Y_SIZE ||
					z < 0 || z >= CONTENTS_Z_SIZE)
				{
					resetFlake = true;
				}
			}
		}

		if (resetFlake)
		{
			item->pos[2] = newMaxs[2] - (newMins[2] - item->pos[2]);
			if (item->pos[2] < newMins[2] || item->pos[2] > newMaxs[2])
			{	// way out of range
				item->pos[2] = ri.flrand(newMins[2], newMaxs[2]);
			}

			item->pos[0] = ri.flrand(newMins[0], newMaxs[0]);
			item->pos[1] = ri.flrand(newMins[1], newMaxs[1]);

			item->velocity[0] = 0.0;
			item->velocity[1] = 0.0;
			item->velocity[2] = ri.flrand(mMaxVelocity[2], mMinVelocity[2]);
			item->flags &= ~PARTICLE_FLAG_RENDER;
		}
		else if (mContents[z][y][x] & CONTENTS_OUTSIDE)
		{
			item->flags |= PARTICLE_FLAG_RENDER;
		}
		else
		{
			item->flags &= ~PARTICLE_FLAG_RENDER;
		}

		item++;
	}
}

const float	attenuation[3] =
{
	1, 0.0f, 0.0004f
};

void CSnowSystem::Render(void)
{
	int			i;
	SParticle	*item;
	vec3_t		origin;

	if (!(mOverallContents & CONTENTS_OUTSIDE))
	{
		return;
	}

	CWorldEffectsSystem::Render();

	VectorAdd(backEnd.viewParms.or.origin, mMinSpread, origin);

	qglColor4f(0.8f, 0.8f, 0.8f, mAlpha);

//	GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE);
	GL_State(GLS_ALPHA);
	qglDisable(GL_TEXTURE_2D);

	if (qglPointParameterfEXT)
	{
		qglPointSize(10.0);
		qglPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, 1.0);
		qglPointParameterfEXT(GL_POINT_SIZE_MAX_EXT, 4.0);
	//	qglPointParameterfEXT(GL_POINT_FADE_THRESHOLD_SIZE_EXT, 3.0);
		qglPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, (float *)attenuation);
	}
	else
	{
		qglPointSize(2.0);
	}

	item = mSnowList;
	qglBegin(GL_POINTS);
	for(i=mMaxSnowflakes;i;i--)
	{
		if (item->flags & PARTICLE_FLAG_RENDER)
		{
			qglVertex3fv(item->pos);
		}
		item++;
	}
	qglEnd();
	qglEnable(GL_TEXTURE_2D);
}

CSnowSystem	*snowSystem = 0;



















CRainSystem::CRainSystem(int maxRain) :
	mMaxRain(maxRain),
	mNextWindGust(0),
	mRainHeight(5),
	mAlpha(0.15f),
	mWindAngle(1.0f),

	mFadeAlpha(0.0f),
	mIsRaining(false)

{
	char			name[256];
	unsigned char	*data, *pos;
	int				width, height;
	int				x, y;

	mSpread[0] = (float)(M_PI*2.0);		// angle spread
	mSpread[1] = 20.0f;			// radius spread
	mSpread[2] = 20.0f;			// z spread

	mMinVelocity[0] = 0.1f;
	mMaxVelocity[0] = -0.1f;
	mMinVelocity[1] = 0.1f;
	mMaxVelocity[1] = -0.1f;
	mMinVelocity[2] = -60.0;
	mMaxVelocity[2] = -50.0;

	mWindDuration = 15;
	mWindLow = 50;
	mWindMin = 0.01f;
	mWindMax = 0.05f;

	mWindChange = 0;
	mWindDirection[0] = mWindDirection[1] = mWindDirection[2] = 0.0;

	mRainList = new SParticle[mMaxRain];

	sprintf(name, "gfx/world/rain.tga");
	R_LoadImage( name, &data, &width, &height );
	if (!data)
	{
		ri.Error (ERR_DROP, "Could not load %s", name);
	}

	pos = data;
	for(y=0;y<height;y++)
	{
		for(x=0;x<width;x++)
		{
			pos[3] = pos[0];
			pos += 4;
		}
	}

	mImage = R_CreateImage(name, data, width, height, false, false, false, GL_CLAMP);
	GL_Bind(mImage);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	Init();
}

CRainSystem::~CRainSystem(void)
{
	delete [] mRainList;
}

void CRainSystem::Init(void)
{
	int			i;
	SParticle	*item;

	item = mRainList;
	for(i=mMaxRain;i;i--)
	{
		item->pos[0] = ri.flrand(0.0, mSpread[0]);
		item->pos[1] = ri.flrand(0.0, mSpread[1]);
		item->pos[2] = ri.flrand(-mSpread[2], 40);
		item->velocity[0] = ri.flrand(mMinVelocity[0], mMaxVelocity[0]);
		item->velocity[1] = ri.flrand(mMinVelocity[1], mMaxVelocity[1]);
		item->velocity[2] = ri.flrand(mMinVelocity[2], mMaxVelocity[2]);
		item++;
	}
}

int CRainSystem::GetIntVariable(int which)
{
	switch(which)
	{
		case CWorldEffect::WORLDEFFECT_PARTICLE_COUNT:
			return mMaxRain;
	}

	return CWorldEffectsSystem::GetIntVariable(which);
}

SParticle *CRainSystem::GetParticleVariable(int which)
{
	switch(which)
	{
		case CWorldEffect::WORLDEFFECT_PARTICLES:
			return mRainList;
	}

	return CWorldEffectsSystem::GetParticleVariable(which);
}

float CRainSystem::GetFloatVariable(int which)
{ 
	switch(which)
	{
		case CRainSystem::RAINSYSTEM_WIND_SPEED:
			return mWindAngle * 75.0;		// pat scaled
	}

	return 0.0;
}

float *CRainSystem::GetVecVariable(int which) 
{ 
	switch(which)
	{
		case CRainSystem::RAINSYSTEM_WIND_DIRECTION:
			return mWindDirection;
	}
	return 0; 
}

bool CRainSystem::Command(const char *command)
{
	char	*token;

	if (CWorldEffectsSystem::Command(command))
	{
		return true;
	}

	token = COM_ParseExt(&command, false);

	if (strcmpi(token, "fog") == 0)
	{	// rain fog
		AddWorldEffect(new CMistyFog2);
		mWindChange = 0;
		return true;
	}
	else if (strcmpi(token, "fall") == 0)
	{	// rain fall ( minVelocity maxVelocity )			default: ( -60 -50 )
		float	data[2];

		if (ParseVector(&command, 2, data))
		{
			mMinVelocity[2] = data[0];
			mMaxVelocity[2] = data[1];
		}
		return true;
	}
	else if (strcmpi(token, "spread") == 0)
	{	// rain spread ( radius height )					default: ( 20 20 )
		ParseVector(&command, 2, &mSpread[1]);
		return true;
	}
	else if (strcmpi(token, "alpha") == 0)
	{	// rain alpha <float>								default: 0.15
		token = COM_ParseExt(&command, false);
		mAlpha = atof(token);
		return true;
	}
	else if (strcmpi(token, "height") == 0)
	{	// rain height <float>								default: 1.5
		token = COM_ParseExt(&command, false);
		mRainHeight = atof(token);
		return true;
	}
	else if (strcmpi(token, "angle") == 0)
	{	// rain angle <float>								default: 1.0
		token = COM_ParseExt(&command, false);
		mWindAngle = atof(token);
		return true;
	}

	return false;
}

void CRainSystem::Update(float elapseTime)
{
	int			i;
	SParticle	*item;
	vec3_t		windDifference;

	if ( elapseTime < 0.0f )
	{
		// sanity check
		elapseTime = 0.0f;
	}

	mWindChange--;

	if (mWindChange < 0)
	{
		mNewWindDirection[0] = 1.0 - (FloatRand() * 2.0);
		mNewWindDirection[1] = 1.0 - (FloatRand() * 2.0);
		mNewWindDirection[2] = 0.0;
		VectorNormalize(mNewWindDirection);
		VectorScale(mNewWindDirection, 0.025f, mWindSpeed);

		mWindChange = 200 + rand() % 250;
//		mWindChange = 10;

		ParmUpdate(CRainSystem::RAINSYSTEM_WIND_DIRECTION);
	}

	VectorSubtract(mNewWindDirection, mWindDirection, windDifference);
	VectorMA(mWindDirection, elapseTime, windDifference);

	CWorldEffectsSystem::Update(elapseTime);

	if (originContents & CONTENTS_OUTSIDE && !(originContents & CONTENTS_WATER))
	{
		mIsRaining = true;
		if (mFadeAlpha < 1.0)
		{
			mFadeAlpha += elapseTime / 2.0;
		}
		if (mFadeAlpha > 1.0)
		{
			mFadeAlpha = 1.0;
		}
	}
	else
	{
		mIsRaining = false;
		if (mFadeAlpha > 0.0)
		{
			mFadeAlpha -= elapseTime / 2.0;
		}

		if (mFadeAlpha <= 0.0)
		{
			return;
		}
	}

	item = mRainList;
	for(i=mMaxRain;i;i--)
	{
		VectorMA(item->pos, elapseTime, item->velocity);

		if (item->pos[2] < -mSpread[2] )
		{
			item->pos[0] = ri.flrand(0.0, mSpread[0]);
			item->pos[1] = ri.flrand(0.0, mSpread[1]);
			item->pos[2] = 40;

			item->velocity[0] = ri.flrand(mMinVelocity[0], mMaxVelocity[0]);
			item->velocity[1] = ri.flrand(mMinVelocity[1], mMaxVelocity[1]);
			item->velocity[2] = ri.flrand(mMinVelocity[2], mMaxVelocity[2]);
		}

		item++;
	}
}

extern vec3_t	mViewAngles, mOrigin;

void CRainSystem::Render(void)
{
	int			i;
	SParticle	*item;
	vec4_t		forward, down, left;
	vec3_t		pos;
//	float		percent;
	float		radius;

	CWorldEffectsSystem::Render();

	if (mFadeAlpha <= 0.0)
	{
		return;
	}

	VectorScale(backEnd.viewParms.or.axis[0], 1, forward);		// forward
	VectorScale(backEnd.viewParms.or.axis[1], 0.2f, left);		// left
	down[0] = 0 - mWindDirection[0] * mRainHeight * mWindAngle;
	down[1] = 0 - mWindDirection[1] * mRainHeight * mWindAngle;
	down[2] = -mRainHeight;

	GL_Bind(mImage);

	GL_State(GLS_ALPHA);
	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_CULL_FACE);

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
    qglTranslatef (backEnd.viewParms.or.origin[0], backEnd.viewParms.or.origin[1],  backEnd.viewParms.or.origin[2]);

	item = mRainList;
	qglBegin(GL_TRIANGLES );
	for(i=mMaxRain;i;i--)
	{
/*		percent = (item->pos[1] -(-20.0)) / (20.0 - (-20.0));
		percent *= forward[2];
		if (percent < 0.0)
		{
			radius = 10 * (percent + 1.0);
		}
		else
		{
			radius = 10 * (1.0 - percent);
		}*/
		radius = item->pos[1];
		if (item->pos[2] < 0.0)
		{
//			radius *= 1.0 - (item->pos[2] / 40.0);
			float alpha = mAlpha * (item->pos[1] / -item->pos[2]);

			if (alpha > mAlpha)
			{
				alpha = mAlpha;
			}
			qglColor4f(1.0, 1.0, 1.0, alpha * mFadeAlpha);
		}
		else
		{
			qglColor4f(1.0, 1.0, 1.0, mAlpha * mFadeAlpha);
//			radius *= 1.0 + (item->pos[2] / 20.0);
		}

		pos[0] = sin(item->pos[0]) * radius + (item->pos[2] * mWindDirection[0] * mWindAngle);
		pos[1] = cos(item->pos[0]) * radius + (item->pos[2] * mWindDirection[1] * mWindAngle);
		pos[2] = item->pos[2];

		qglTexCoord2f(1.0, 0.0);
		qglVertex3f(pos[0],
					pos[1],
					pos[2]);

		qglTexCoord2f(0.0, 0.0);
		qglVertex3f(pos[0] + left[0],
					pos[1] + left[1],
					pos[2] + left[2]);
		
		qglTexCoord2f(0.0, 1.0);
		qglVertex3f(pos[0] + down[0] + left[0],
					pos[1] + down[1] + left[1],
					pos[2] + down[2] + left[2]);
		item++;
	}
	qglEnd();

	qglEnable(GL_CULL_FACE);

	qglPopMatrix();
}






CRainSystem	*rainSystem = 0;


void R_InitWorldEffects(void)
{
	if (rainSystem)
	{
		delete rainSystem;
	}

	if (snowSystem)
	{
		delete snowSystem;
	}
}

void R_ShutdownWorldEffects(void)
{
	if (rainSystem)
	{
		delete rainSystem;
		rainSystem = 0;
	}
	if (snowSystem)
	{
		delete snowSystem;
		snowSystem = 0;
	}
}

void SetViewportAndScissor( void ) ;

void RB_RenderWorldEffects(void)
{
	float					elapseTime = backEnd.refdef.frametime / 1000.0;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL || !tr.world || CL_IsRunningInGameCinematic()) 
	{	//  no world rendering or no world
		return;
	}

	SetViewportAndScissor();
	qglMatrixMode(GL_MODELVIEW);
//	qglPushMatrix();
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );

	originContents = ri.CM_PointContents(backEnd.viewParms.or.origin, 0);

	if (rainSystem)
	{
		rainSystem->Update(elapseTime);
		rainSystem->Render();
	}

	if (snowSystem)
	{
		snowSystem->Update(elapseTime);
		snowSystem->Render();
	}

//	qglMatrixMode(GL_MODELVIEW);
//	qglPopMatrix();
}

//	console commands for r_we
//
//	SNOW
//		snow init <particles>
//		snow remove
//		snow alpha <float>											default: 0.09
//		snow spread ( minX minY minZ ) ( maxX maxY maxZ )			default: ( -600 -600 -200 ) ( 600 600 250 )
//		snow velocity ( minX minY minZ ) ( maxX maxY maxZ )			default: ( -15 -15 -20 ) ( 15 15 -70 )
//		snow blowing duration <int>									default: 2
//		snow blowing low <int>										default: 3
//		snow blowing velocity ( min max )							default: ( 30 70 )
//		snow blowing size ( minX minY minZ )						default: ( 1000 300 300 )
//		snow wind ( windOriginX windOriginY windOriginZ ) ( windVelocityX windVelocityY windVelocityZ ) ( sizeX sizeY sizeZ )
//		snow fog
//		snow fog density <alpha>									default: 0.3
//
//	RAIN
//		rain init <particles>
//		rain remove
//		rain fog
//		rain fog density <alpha>									default: 0.3
//		rain fall ( minVelocity maxVelocity )						default: ( -60 -50 )
//		rain spread ( radius height )								default: ( 20 20 )
//		rain alpha <float>											default: 0.1
//		rain height <float>											default: 5
//		rain angle <float>											default: 1.0
//
//	DEBUG
//		debug wind

void R_WorldEffectCommand(const char *command)
{
	const char	*token, *origCommand;

	token = COM_ParseExt(&command, false);

	if (strcmpi(token, "snow") == 0)
	{
		origCommand = command;

		token = COM_ParseExt(&command, false);
		if (strcmpi(token, "init") == 0)
		{	//	snow init <particles>
			token = COM_ParseExt(&command, false);
			if (snowSystem)
			{
				delete snowSystem;
			}
			snowSystem = new CSnowSystem(atoi(token));
		}
		else if (strcmpi(token, "remove") == 0)
		{	//	snow remove
			if (snowSystem)
			{
				delete snowSystem;
				snowSystem = 0;
			}
		}
		else if (snowSystem)
		{
			snowSystem->Command(origCommand);
		}
	}
	else if (strcmpi(token, "rain") == 0)
	{
		origCommand = command;

		token = COM_ParseExt(&command, false);
		if (strcmpi(token, "init") == 0)
		{	//	rain init <particles>
			token = COM_ParseExt(&command, false);
			if (rainSystem)
			{
				delete rainSystem;
			}
			rainSystem = new CRainSystem(atoi(token));
		}
		else if (strcmpi(token, "remove") == 0)
		{	//	rain remove
			if (rainSystem)
			{
				delete rainSystem;
				rainSystem = 0;
			}
		}
		else if (rainSystem)
		{
			rainSystem->Command(origCommand);
		}
	}
	else if (strcmpi(token, "debug") == 0)
	{
		token = COM_ParseExt(&command, false);
		if (strcmpi(token, "wind") == 0)
		{
			debugShowWind = !debugShowWind;
		}
		else if (strcmpi(token, "blah") == 0)
		{
			R_WorldEffectCommand("snow init 1000");
			R_WorldEffectCommand("snow alpha 1");
			R_WorldEffectCommand("snow fog");
		}
	}
}

void R_WorldEffect_f(void)
{
	char		temp[2048];

	ri.ArgsBuffer(temp, sizeof(temp));
	R_WorldEffectCommand(temp);
}

bool R_GetWindVector(vec3_t windVector)
{
	if (rainSystem)
	{
		VectorCopy(rainSystem->GetVecVariable(CRainSystem::RAINSYSTEM_WIND_DIRECTION), windVector);
		return true;
	}

	if (snowSystem)
	{
		VectorCopy(snowSystem->GetVecVariable(CRainSystem::RAINSYSTEM_WIND_DIRECTION), windVector);
		return true;
	}


	return false;
}

bool R_GetWindSpeed(float &windSpeed)
{
	if (rainSystem)
	{
		windSpeed = rainSystem->GetFloatVariable(CRainSystem::RAINSYSTEM_WIND_SPEED);
		return true;
	}

	return false;
}

bool R_IsRaining()
{
	if (rainSystem)
	{
		return rainSystem->IsRaining();
	}
	return false;
}

bool R_IsSnowing()
{
	if (snowSystem)
	{
		return snowSystem->IsSnowing();
	}
	return false;
}
