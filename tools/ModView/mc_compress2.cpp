// Filename:-	mc_compress2
//
//
#include <math.h>
#include <windows.h>

#include "mc_compress2.h"
#include <assert.h>


extern 
#ifdef _CARCASS 
"C"
#endif
char *va(char *format, ...);



typedef float vec3_t[3];
#ifndef DotProduct
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#endif

typedef unsigned char byte;
typedef unsigned short word;

static const float gfEpsilon = 1e-03f;
#define DATA(f0,f1,f2) {f0,f1,f2}

/*static vec3_t v3VecTable[]=
{
};
*/			    
struct v3Struct_t
{
	float f[3];

	bool operator < (const v3Struct_t& _X) const {return (memcmp(f,_X.f,sizeof(f))<0);}
};

#pragma warning ( disable : 4786)
#include <map>
#include <string>
#pragma warning ( disable : 4786)
using namespace std;
typedef map<v3Struct_t,int> Table_t;
Table_t Table;

typedef map <string, float> Thing_t;
							Thing_t Thing;
void crap(void)
{
	OutputDebugString(va("Table size %d\n",Thing.size()));
	for (Thing_t::iterator it = Thing.begin(); it!=Thing.end(); ++it)
	{
		const string *str = &((*it).first);
		float f   = (*it).second;

		OutputDebugString(va("(%s) %f\n",str->c_str(),f));
	}

/*	OutputDebugString(va("Table size %d\n",Table.size()));
	int iHighestUsageCount = 0;

	int iEntry=0;
	for (Table_t::iterator it = Table.begin(); it!= Table.end(); ++it)
	{
		v3Struct_t v3 = (*it).first;
		//OutputDebugString(va("(%d) %f %f %f\n",iEntry,v3.f[0],v3.f[1],v3.f[2]));
		int iUsage = (*it).second;
		if (iUsage > iHighestUsageCount)
		{
			iHighestUsageCount = iUsage;
		}
		iEntry++;
	}

	OutputDebugString(va("Highest usage count = %d\n",iHighestUsageCount));
	*/
}


inline float ACos (float fValue)
{
    if ( -1.0f < fValue )
    {
        if ( fValue < 1.0f )
            return (float)acos(fValue);
        else
            return 0.0f;
    }
    else
    {
        return (float) 3.14159265358979323846264;//PI;
    }
}

static void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) 
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}



static inline word SquashFloat(float f)
{
//	assert(f >= -2 && f <= 2);
	if (!(f >= -2 && f <= 2))
	{
#ifdef _CARCASS
		// crap for carcass so I collect all bad errors together before finally exiting
		//
		extern bool gbSquashFloatError;
		if (!gbSquashFloatError)
		{
			printf("SquashFloat(): Fatal Error: f = %f ( range permitted: -2..2 )\n",f);
			gbSquashFloatError = true;
		}
#elif _MODVIEW
		assert(0);
#endif
		return 0;
	}

	f+= 2.0f;	// range 0..4
	f*= 16383.0f;

	int iVal = f;
	assert(iVal <= 65535);
	return (word) iVal&65535;
}

static inline float UnSquashFloat(word w)
{
	float f = w;
	f/=16383.0f;	//32767.0f;
	f-=2.0f;		//1.0f;

	assert(f >= -2.0f && f<= 2.0f);
	return f;
}

static inline float UnSquashXlatFloat(word w)
{
	float 
	f = w;
	f/=64;
	f-=512;

	return f;
}

static inline word SquashXlatFloat(float f)
{
//	assert(f >= -511 && f <= 511);
	if (!(f >= -511 && f <= 511))
	{
#ifdef _CARCASS
		// crap for carcass so I collect all bad errors together before finally exiting
		//
		extern bool gbSquashFloatError;
		if (!gbSquashFloatError)
		{
			printf("SquashXlatFloat(): Fatal Error: bone translation = %f ( range permitted: -511..511 )\n( Recompile this model using carcass_prequat.exe instead )\n",f);
			gbSquashFloatError = true;
		}
#elif _MODVIEW
		assert(0);
#endif
		return 0;
	}
	f+= 512;
	f*=64;

#ifdef _DEBUG
	int iVal = (int)f;
	assert( iVal >= 0 && iVal <= 65535);
#endif

	word w = (word) f;
	return w;
}

class Xlat
{
public:
	float x,y,z;

	Xlat();			// do nothing in constructor

	const	word *FromComp	(const byte *pComp);
			word *ToComp	(byte *pComp);
};

const word *Xlat::FromComp(const byte *pComp)
{
	const word *pwIn = (word *) pComp;

	x= UnSquashXlatFloat(*pwIn++);
	y= UnSquashXlatFloat(*pwIn++);
	z= UnSquashXlatFloat(*pwIn++);

	return pwIn;
}

word *Xlat::ToComp(byte *pComp)
{
	word *pwOut = (word *) pComp;

	*pwOut++ = SquashXlatFloat(x);
	*pwOut++ = SquashXlatFloat(y);
	*pwOut++ = SquashXlatFloat(z);

	return pwOut;
}


class Quaternion
{
public:
	float w,x,y,z;

	Quaternion();	// do nothing in default constructor
	Quaternion(float fW, float fX, float fY, float fZ);
	Quaternion(const Quaternion& rkQ);

	const	word *FromComp	(const byte *pComp);
			word *ToComp	(byte *pComp) const;

			void  To3x4		(float kRot[3][4]) const;
			float Dot		(const Quaternion& Q) const;

	Quaternion Slerp		(float fT, const Quaternion& rkP, const Quaternion& rkQ);

	Quaternion& operator=	(const Quaternion& rkQ);
	Quaternion  operator+	(const Quaternion& rkQ) const;
	Quaternion  operator-	(const Quaternion& rkQ) const;
	Quaternion  operator*	(const Quaternion& rkQ) const;
	Quaternion  operator*	(float fScalar) const;
};


Quaternion::Quaternion (void)
{
}

Quaternion::Quaternion (float fW, float fX, float fY, float fZ)
{
    w = fW;
    x = fX;
    y = fY;
    z = fZ;
}

Quaternion::Quaternion (const Quaternion& rkQ)
{
    w = rkQ.w;
    x = rkQ.x;
    y = rkQ.y;
    z = rkQ.z;
}


Quaternion& Quaternion::operator= (const Quaternion& rkQ)
{
    w = rkQ.w;
    x = rkQ.x;
    y = rkQ.y;
    z = rkQ.z;
    return *this;
}

Quaternion Quaternion::operator+ (const Quaternion& rkQ) const
{
    return Quaternion(w+rkQ.w,x+rkQ.x,y+rkQ.y,z+rkQ.z);
}

Quaternion Quaternion::operator- (const Quaternion& rkQ) const
{
    return Quaternion(w-rkQ.w,x-rkQ.x,y-rkQ.y,z-rkQ.z);
}

Quaternion Quaternion::operator* (const Quaternion& rkQ) const
{
    // NOTE:  Multiplication is not generally commutative, so in most
    // cases p*q != q*p.

    return Quaternion
    (
		w*rkQ.w-x*rkQ.x-y*rkQ.y-z*rkQ.z,
		w*rkQ.x+x*rkQ.w+y*rkQ.z-z*rkQ.y,
		w*rkQ.y+y*rkQ.w+z*rkQ.x-x*rkQ.z,
		w*rkQ.z+z*rkQ.w+x*rkQ.y-y*rkQ.x
    );
}

Quaternion Quaternion::operator* (float fScalar) const
{
    return Quaternion(fScalar*w,fScalar*x,fScalar*y,fScalar*z);
}

float Quaternion::Dot(const Quaternion& Q) const
{
    return w*Q.w + x*Q.x + y*Q.y + z*Q.z;
}


const word *Quaternion::FromComp(const byte *pComp)
{
	const word *pwIn = (word *) pComp;
	w = UnSquashFloat(*pwIn++);
	x = UnSquashFloat(*pwIn++);
	y = UnSquashFloat(*pwIn++);
	z = UnSquashFloat(*pwIn++);

	return pwIn;
}

word *Quaternion::ToComp(byte *pComp) const
{
	word *pwOut = (word *) pComp;
	*pwOut++ = SquashFloat(w);
	*pwOut++ = SquashFloat(x);
	*pwOut++ = SquashFloat(y);
	*pwOut++ = SquashFloat(z);	// 8 bytes

#ifdef _DEBUG
	// measure biggest deviation...
	//
#define __BLAH(_l)							\
	static float fMin ## _l = 0.0f, fMax ## _l = 0.0f;	\
	if ( _l < fMin ## _l){					\
		fMin ## _l = _l;					\
		Thing["fMin" #_l] = _l;				\
	}										\
	if ( _l > fMax ## _l){					\
		fMax ## _l = _l;					\
		Thing["fMax" #_l] = _l;				\
	}
	__BLAH(w);
	__BLAH(x);
	__BLAH(y);
	__BLAH(z);
#endif

	return pwOut;
}

void Quaternion::To3x4(float kRot[3][4]) const
{
    float fTx  = 2.0f*x;
    float fTy  = 2.0f*y;
    float fTz  = 2.0f*z;
    float fTwx = fTx*w;
    float fTwy = fTy*w;
    float fTwz = fTz*w;
    float fTxx = fTx*x;
    float fTxy = fTy*x;
    float fTxz = fTz*x;
    float fTyy = fTy*y;
    float fTyz = fTz*y;
    float fTzz = fTz*z;

    kRot[0][0] = 1.0f-(fTyy+fTzz);
    kRot[0][1] = fTxy-fTwz;
    kRot[0][2] = fTxz+fTwy;
    kRot[1][0] = fTxy+fTwz;
    kRot[1][1] = 1.0f-(fTxx+fTzz);
    kRot[1][2] = fTyz-fTwx;
    kRot[2][0] = fTxz-fTwy;
    kRot[2][1] = fTyz+fTwx;
    kRot[2][2] = 1.0f-(fTxx+fTyy);
}


//----------------------------------------------------------------------------
Quaternion Quaternion::Slerp (float fT, const Quaternion& rkP,
    const Quaternion& rkQ)
{
    float fCos = rkP.Dot(rkQ);
    float fAngle = ACos(fCos);

    if ( fabs(fAngle) < gfEpsilon )
        return rkP;

    float fSin = sin(fAngle);
    float fInvSin = 1.0f/fSin;
    float fCoeff0 = sin((1.0f-fT)*fAngle)*fInvSin;
    float fCoeff1 = sin(fT*fAngle)*fInvSin;
    return rkP*fCoeff0 + rkQ*fCoeff1;
}



void Quaternion_3x4ToComp (const float kRot[3][4], unsigned char *pComp)
{
	Quaternion Q;

    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    float fTrace = kRot[0][0]+kRot[1][1]+kRot[2][2];
    float fRoot;

    if ( fTrace > 0.0f )
    {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = sqrt(fTrace + 1.0f);  // 2w
        Q.w = 0.5f*fRoot;
        fRoot = 0.5f/fRoot;  // 1/(4w)
        Q.x = (kRot[2][1]-kRot[1][2])*fRoot;
        Q.y = (kRot[0][2]-kRot[2][0])*fRoot;
        Q.z = (kRot[1][0]-kRot[0][1])*fRoot;
    }
    else
    {
        // |w| <= 1/2
        static int s_iNext[3] = { 1, 2, 0 };
        int i = 0;
        if ( kRot[1][1] > kRot[0][0] )
            i = 1;
        if ( kRot[2][2] > kRot[i][i] )
            i = 2;
        int j = s_iNext[i];
        int k = s_iNext[j];

        fRoot = sqrt(kRot[i][i]-kRot[j][j]-kRot[k][k] + 1.0f);
        float* apkQuat[3] = { &Q.x, &Q.y, &Q.z };
        *apkQuat[i] = 0.5f*fRoot;
        fRoot = 0.5f/fRoot;
        Q.w = (kRot[k][j]-kRot[j][k])*fRoot;
        *apkQuat[j] = (kRot[j][i]+kRot[i][j])*fRoot;
        *apkQuat[k] = (kRot[k][i]+kRot[i][k])*fRoot;
    }

	// copy out to dest arg...
	//
	word *pwOut = Q.ToComp(pComp);

	// xlat...
	//
	*pwOut++ = SquashXlatFloat(kRot[0][3]);
	*pwOut++ = SquashXlatFloat(kRot[1][3]);
	*pwOut++ = SquashXlatFloat(kRot[2][3]);
}

//----------------------------------------------------------------------------
void Quaternion_3x4FromComp (float kRot[3][4], const byte *pComp)
{
	Quaternion Q;
	
	const word *pwIn =	Q.FromComp(pComp);
						Q.To3x4(kRot);

	// xlat...
	//
	kRot[0][3] = UnSquashXlatFloat(*pwIn++);
	kRot[1][3] = UnSquashXlatFloat(*pwIn++);
	kRot[2][3] = UnSquashXlatFloat(*pwIn++);
}



void QuatSlerpCompTo3x4(	float fLerp0_1,
							const byte *pComp0,
							const byte *pComp1,
							float fDestMat[3][4]
							)
{
	assert(fLerp0_1 >= 0.0f && fLerp0_1 <= 1.0f);

	Quaternion Q0,Q1;

	float tx0,ty0,tz0;	// xlats
	float tx1,ty1,tz1;	//

	// unpack 0...
	//
	const word *
	pwIn = Q0.FromComp(pComp0);
	//
	// and xlat...
	//
	tx0 = UnSquashXlatFloat(*pwIn++);
	ty0 = UnSquashXlatFloat(*pwIn++);
	tz0 = UnSquashXlatFloat(*pwIn++);

	// unpack 1...
	//
	pwIn = Q1.FromComp(pComp1);
	//
	// and xlat...
	//
	tx1 = UnSquashXlatFloat(*pwIn++);
	ty1 = UnSquashXlatFloat(*pwIn++);
	tz1 = UnSquashXlatFloat(*pwIn++);

	// slerp...
	//
	Quaternion	Q2;
				Q2 = Q2.Slerp(1.0f-fLerp0_1, Q0, Q1);
				Q2.To3x4(fDestMat);

	// lerp the xlat...
	//
	fDestMat[0][3] = (fLerp0_1 * tx0) + ((1.0 - fLerp0_1) * tx1);
	fDestMat[1][3] = (fLerp0_1 * ty0) + ((1.0 - fLerp0_1) * ty1);
	fDestMat[2][3] = (fLerp0_1 * tz0) + ((1.0 - fLerp0_1) * tz1);
}




// outputs 14 bytes
//
void MC_Compress2(const float mat[3][4],unsigned char *comp)
{
	Quaternion_3x4ToComp(mat, comp);
}


void MC_UnCompress2(float mat[3][4],const unsigned char * comp)
{
	Quaternion_3x4FromComp( mat, comp);
}
