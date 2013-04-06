#include "matrix4.h"

extern const float Pi=3.1415926535f;
extern const float Half_Pi=Pi/2.0;


#define MAX_MATRIX (12)

#define MATRIX_DEBUG 0

void BackSub(float a[][MAX_MATRIX],int *indx,float *b,int sz)
{
    int i,j,ii=-1,ip;
    float sum;
    for (i=0;i<sz;i++)
    {
		ip=indx[i];
		sum=b[ip];
        b[ip]=b[i];
        if (ii>=0)
        {
            for (j=ii;j<i;j++)
                sum-=a[i][j]*b[j];
        }
        else if (sum!=0.0)
        {
            ii=i;
        }
        b[i]=sum;
    }
    for (i=sz-1;i>=0;i--)
    {
        sum=b[i];
        for (j=i+1;j<sz;j++)
            sum-=a[i][j]*b[j];
        b[i]=sum/a[i][i];
    }
}

#define EPS_MATRIX (1E-15)

int LUDecomposition(float a[][MAX_MATRIX],int indx[],int sz,float *d)
{
    int i,imax,j,k;
    float big,dum,sum,temp;
    float vv[MAX_MATRIX];
	*d=1;
    for (i=0;i<sz;i++)
    {
        big=0.;
        for (j=0;j<sz;j++)
            if ((temp=fabs(a[i][j]))>big)
                    big=temp;
        if (big<EPS_MATRIX)
			return 0;
        vv[i]=1.0/big;
    }
	imax = 0;//should have a def value just to make sure
    for (j=0;j<sz;j++)
    {
        for (i=0;i<j;i++)
        {
            sum=a[i][j];
            for (k=0;k<i;k++)
                sum-=a[i][k]*a[k][j];
            a[i][j]=sum;
        }
        big=0.;
        for (i=j;i<sz;i++)
        {
            sum=a[i][j];
            for (k=0;k<j;k++)
                sum-=a[i][k]*a[k][j];
            a[i][j]=sum;
            if ((dum=vv[i]*fabs(sum))>=big)
            {
                big=dum;
                imax=i;
            }
        }
        if (j!=imax)
        {
            for (k=0;k<sz;k++)
            {
                dum=a[imax][k];
                a[imax][k]=a[j][k];
                a[j][k]=dum;
            }
            vv[imax]=vv[j];
			*d=-(*d);
        }
        indx[j]=imax;
        if (fabs(a[j][j])<EPS_MATRIX)
			return 0;
        if (j!=sz-1)
        {
            dum=1.0/a[j][j];
            for (i=j+1;i<sz;i++)
                    a[i][j]*=dum;
        }
    }
	return 1;
}


void BackSub4(float a[][4],int *indx,float *b)
{
	const int sz=4;
    int i,j,ii=-1,ip;
    float sum;
    for (i=0;i<sz;i++)
    {
		ip=indx[i];
		sum=b[ip];
        b[ip]=b[i];
        if (ii>=0)
        {
            for (j=ii;j<i;j++)
                sum-=a[i][j]*b[j];
        }
        else if (sum!=0.0)
        {
            ii=i;
        }
        b[i]=sum;
    }
    for (i=sz-1;i>=0;i--)
    {
        sum=b[i];
        for (j=i+1;j<sz;j++)
            sum-=a[i][j]*b[j];
        b[i]=sum/a[i][i];
    }
}


int LUDecomposition4(float a[4][4],int indx[4],float *d)
{
	const int sz=4;
    int i,imax,j,k;
    float big,dum,sum,temp;
    float vv[MAX_MATRIX];
	*d=1;
    for (i=0;i<sz;i++)
    {
        big=0.;
        for (j=0;j<sz;j++)
            if ((temp=fabs(a[i][j]))>big)
                    big=temp;
        if (big<EPS_MATRIX)
			return 0;
        vv[i]=1.0/big;
    }
	imax = 0;//should have a def value just to make sure
    for (j=0;j<sz;j++)
    {
        for (i=0;i<j;i++)
        {
            sum=a[i][j];
            for (k=0;k<i;k++)
                sum-=a[i][k]*a[k][j];
            a[i][j]=sum;
        }
        big=0.;
        for (i=j;i<sz;i++)
        {
            sum=a[i][j];
            for (k=0;k<j;k++)
                sum-=a[i][k]*a[k][j];
            a[i][j]=sum;
            if ((dum=vv[i]*fabs(sum))>=big)
            {
                big=dum;
                imax=i;
            }
        }
        if (j!=imax)
        {
            for (k=0;k<sz;k++)
            {
                dum=a[imax][k];
                a[imax][k]=a[j][k];
                a[j][k]=dum;
            }
            vv[imax]=vv[j];
			*d=-(*d);
        }
        indx[j]=imax;
        if (fabs(a[j][j])<EPS_MATRIX)
			return 0;
        if (j!=sz-1)
        {
            dum=1.0/a[j][j];
            for (i=j+1;i<sz;i++)
                    a[i][j]*=dum;
        }
    }
	return 1;
}


int GetTextureSystem(
  const Vect3 &p0,const Vect3 &uv0, //vertex 0
  const Vect3 &p1,const Vect3 &uv1, //vertex 1
  const Vect3 &p2,const Vect3 &uv2, //vertex 2
  const Vect3 &n, // normal vector for triangle
  Vect3 &M, // returned gradient wrt u
  Vect3 &N, // returned gradient wrt v
  Vect3 &P) // returned location in world space where u=v=0
/*
	for and point q in world space, the uv coords are
	u=(q-P).M
	v=(q-P).N
*/
{
        static float a[6][MAX_MATRIX],ans[6];
        static int indx[6];
        int i,j;

		for (i=0;i<6;i++)
			for (j=0;j<6;j++)
				a[i][j]=0.0f;
		for (i=0;i<3;i++)
			a[0][i]=p1[i]-p0[i];
		for (i=0;i<3;i++)
			a[1][i]=p2[i]-p0[i];
		for (i=0;i<3;i++)
			a[2][i+3]=p1[i]-p0[i];
		for (i=0;i<3;i++)
			a[3][i+3]=p2[i]-p0[i];
		for (i=0;i<3;i++)
			a[4][i]=n[i];
		for (i=0;i<3;i++)
			a[5][i+3]=n[i];

		for (i=0;i<3;i++)
		{
			M[i]=0.0f;
			N[i]=0.0f;
			P[i]=0.0f;
		}
		float det;
		if (!LUDecomposition(a,indx,6,&det))
			return 0;
		else
		{
			ans[0]=uv1[0]-uv0[0];
			ans[1]=uv2[0]-uv0[0];
			ans[2]=uv1[1]-uv0[1];
			ans[3]=uv2[1]-uv0[1];
			ans[4]=0.0f;
			ans[5]=0.0f;
			BackSub(a,indx,ans,6);
			for (i=0;i<3;i++)
			{
				M[i]=ans[i];
				N[i]=ans[i+3];
			}

			ans[0]=p0[0]*M[0]+p0[1]*M[1]+p0[2]*M[2]-uv0[0];
			ans[1]=p0[0]*N[0]+p0[1]*N[1]+p0[2]*N[2]-uv0[1];
			ans[2]=p0[0]*n[0]+p0[1]*n[1]+p0[2]*n[2];
			a[0][0]=M[0];	
			a[0][1]=M[1];	
			a[0][2]=M[2];	
			a[1][0]=N[0];	
			a[1][1]=N[1];	
			a[1][2]=N[2];	
			a[2][0]=n[0];	
			a[2][1]=n[1];	
			a[2][2]=n[2];	
			if (!LUDecomposition(a,indx,3,&det))
				return 0;
			else		
			{
				BackSub(a,indx,ans,3);
				P[0]=ans[0];
				P[1]=ans[1];
				P[2]=ans[2];
			}
		}
		return 1;
}


void Matrix4::Inverse(const Matrix4 &old)
{
	if (old.flags&MATFLAG_IDENTITY)
		Identity();
	else
	{
		float t[4],d;
		int i,indx[4];
		Matrix4 lu=old;
		if (!LUDecomposition4(lu.m,indx,&d))
			assert(0);

		t[0]=1;
		t[1]=0;
		t[2]=0;
		t[3]=0;

		BackSub4(lu.m,indx,t);
		for(i=0;i<4;i++)
			m[i][0]=t[i];

		t[0]=0;
		t[1]=1;
		t[2]=0;
		t[3]=0;
		BackSub4(lu.m,indx,t);
		for(i=0;i<4;i++)
			m[i][1]=t[i];

		t[0]=0;
		t[1]=0;
		t[2]=1;
		t[3]=0;
		BackSub4(lu.m,indx,t);
		for(i=0;i<4;i++)
			m[i][2]=t[i];

		t[0]=0;
		t[1]=0;
		t[2]=0;
		t[3]=1;
		BackSub4(lu.m,indx,t);
		for(i=0;i<4;i++)
			m[i][3]=t[i];
		flags=0;
	}
}

void Matrix4::OrthoNormalInverse(const Matrix4 &old)
{
	flags=0;
	int i,j;
	for (i=0;i<3;i++)
	{
		for (j=0;j<3;j++)
			m[i][j]=old.m[j][i];
		m[i][j]=0.0f;
	}
	m[3][3]=1.0f;
	Vect3 t,tt;
	old.GetRow(3,t);
	XFormVect(tt,t);
	tt*=-1.0f;
	SetRow(3,tt);
#if 0
	{
		Matrix4 test;
		test.Inverse(old);
		tt=0.0f;
	}
#endif
}

void Matrix4::FindFromPoints(const Vect3 base1[4],const Vect3 base2[4])
{
	float sol[12][12];
	float o[12];
	int indx[12];
	float d;
	int i,j,k;
	for (i=0;i<12;i++)
	{
		o[i]=base2[i/3][i%3];
		for (j=0;j<12;j++)
			sol[i][j]=0.0f;
	}
	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
			for (k=0;k<3;k++)
				sol[i*3+j][j*4+k]=base1[i][k];
			sol[i*3+j][j*4+3]=1.0;
		}
	}
	if (!LUDecomposition(sol,indx,12,&d))
		assert(0);
	BackSub(sol,indx,o,12);
	for(i=0;i<12;i++)
		m[i%4][i/4]=o[i];
	m[0][3]=0.0f;
	m[1][3]=0.0f;
	m[2][3]=0.0f;
	m[3][3]=1.0f;
	CalcFlags();
}

void Matrix4::MakeEquiscalar()
{
	float l,lc[3];
	m[0][3]=0.0f;
	m[1][3]=0.0f;
	m[2][3]=0.0f;
	m[3][3]=1.0f;
	int i,j;
	l=0.0f;
	for (i=0;i<3;i++)
	{
		lc[i]=0.0f;
		for (j=0;j<3;j++)
			lc[i]+=m[j][i]*m[j][i];
		lc[i]=sqrt(lc[i]);
		l+=lc[i];
	}
	l/=3.0f;
	for (i=0;i<3;i++)
	{
		lc[i]=l/lc[i];
		for (j=0;j<3;j++)
			m[j][i]*=lc[i];
	}
	CalcFlags();
}
	
Matrix4::Matrix4(const Matrix4 &o)
{
	int i;
	float *f;
	const float *of;
	for (f=m[0],of=o.m[0],i=0;i<16;i++,f++,of++)
		*f=*of;
	flags=o.flags;
}

#define MATRIX_TOL (1E-4f)

bool Matrix4::IntegrityCheck() const
{
#if MATRIX_DEBUG
	if (flags&MATFLAG_IDENTITY)
	{
		int i,j;
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				if (i==j)
					assert(fabs(1.0-m[i][j])<MATRIX_TOL);
				else
					assert(fabs(m[i][j])<MATRIX_TOL);
			}
		}
	}
#endif
	return true;
}

int Matrix4::CalcFlags()
{
	int i,j;
	flags=0;
	for (i=0;i<4;i++)
	{
		for (j=0;j<4;j++)
		{
			if (i==j)
			{
				if (fabs(1.0-m[i][j])>MATRIX_TOL)
					return 0;
			}
			else
			{
				if (fabs(m[i][j])>MATRIX_TOL)
					return 0;
			}
		}
	}
	flags=MATFLAG_IDENTITY;
	return flags;
}


void Matrix4::Identity()
{
	int i;
	float *f;
	for (f=m[0],i=0;i<16;i++,f++)
		*f=0;
	for (i=0;i<4;i++)
		m[i][i]=1.0;;
	flags=MATFLAG_IDENTITY;
}

float Matrix4::MaxAbsElement()
{
	if (flags&MATFLAG_IDENTITY)
	{
		assert(IntegrityCheck());
		return 1.0f;
	}
	int i;
	float best=-1,*f;
	for (f=m[0],i=0;i<16;i++,f++)
	{
		if (fabs(*f)>best)
			best=fabs(*f);
	}
	return best;
}

void Matrix4::Zero()
{
	int i;
	float *f;
	for (f=m[0],i=0;i<16;i++,f++)
		*f=0;
	flags=0;
}

float Matrix4::Det()
{
	if (flags&MATFLAG_IDENTITY)
	{
		assert(IntegrityCheck());
		return 1.0f;
	}
	float d;
	int i,indx[4];
	Matrix4 lu=*this;
	if (!LUDecomposition4(lu.m,indx,&d))
		assert(0);
	for (i=0;i<4;i++)
		d*=lu.m[i][i];
	return d;
}


void Matrix4::Translate(const float tx,const float ty,const float tz)
{
	int i;
	float *f;
	for (f=m[0],i=0;i<12;i++,f++)
		*f=0;
	for (i=0;i<4;i++)
		m[i][i]=1.0;;
	m[3][0]=tx;
	m[3][1]=ty;
	m[3][2]=tz;
	CalcFlags();
}

void Matrix4::Translate(const Vect3 &t)
{
	int i;
	float *f;
	for (f=m[0],i=0;i<12;i++,f++)
		*f=0;
	for (i=0;i<4;i++)
		m[i][i]=1.0;;
	m[3][0]=t[0];
	m[3][1]=t[1];
	m[3][2]=t[2];
	CalcFlags();
}

void Matrix4::Rotate(int axis,const float theta)
{
	int i;
	float *f,s,c;
	for (f=m[0],i=0;i<16;i++,f++)
		*f=0;
	for (i=0;i<4;i++)
		m[i][i]=1.0;
	s=sin(theta);
	c=cos(theta);
	switch(axis)
	{
	case 0:
		m[1][1]=c;
		m[1][2]=-s;
		m[2][1]=s;
		m[2][2]=c;
		break;
	case 1:
		m[0][0]=c;
		m[0][2]=s;
		m[2][0]=-s;
		m[2][2]=c;
		break;
	case 2:
		m[0][0]=c;
		m[0][1]=-s;
		m[1][0]=s;
		m[1][1]=c;
		break;
	}
	CalcFlags();
}

void Matrix4::Rotate(const float rx,const float ry,const float rz)
{
	Matrix4 x,y,z,t;
	x.Rotate(0,rx);
	y.Rotate(1,ry);
	z.Rotate(2,rz);
	t.Concat(y,z);
	Concat(x,t);
}

void Matrix4::Rotate(const Vect3 v)
{
	Matrix4 x,y,z,t;

	x.Rotate(0, v[0]);
	y.Rotate(1, v[1]);
	z.Rotate(2, v[2]);

	t.Concat(y, z);
	Concat(x, t);
}

void Matrix4::Scale(const float sx,const float sy,const float sz)
{
	int i;
	float *f;
	for (f=m[0],i=0;i<16;i++,f++)
		*f=0;
	assert(fabs(sx)>1E-10);
	assert(fabs(sy)>1E-10);
	assert(fabs(sz)>1E-10);
	m[0][0]=sx;
	m[1][1]=sy;
	m[2][2]=sz;
	m[3][3]=1.0;
	CalcFlags();
}

void Matrix4::Scale(const float sx)
{
	int i;
	float *f;
	for (f=m[0],i=0;i<16;i++,f++)
		*f=0;
	assert(fabs(sx)>1E-10);
	m[0][0]=sx;
	m[1][1]=sx;
	m[2][2]=sx;
	m[3][3]=1.0;
	CalcFlags();
}

void Matrix4::Interp(const Matrix4 &m1,float s1,const Matrix4 &m2,float s2)
{
	if ((m1.flags&MATFLAG_IDENTITY)&&(m2.flags&MATFLAG_IDENTITY))
	{
		assert(m1.IntegrityCheck());
		assert(m2.IntegrityCheck());
		Identity();
		return;
	}
	int i;
	const float *f1;
	const float *f2;
	float *fd;
	for (f1=m1.m[0],f2=m2.m[0],fd=m[0],i=0;i<16;i++,f1++,f2++,fd++)
		*fd=s1*(*f1)+s2*(*f2);
	flags=0;
}

void Matrix4::Interp(const Matrix4 &m1,const Matrix4 &m2,float s1)
{
	if ((m1.flags&MATFLAG_IDENTITY)&&(m2.flags&MATFLAG_IDENTITY))
	{
		assert(m1.IntegrityCheck());
		assert(m2.IntegrityCheck());
		Identity();
		return;
	}
	int i;
	const float *f1;
	const float *f2;
	float *fd;
	for (f1=m1.m[0],f2=m2.m[0],fd=m[0],i=0;i<16;i++,f1++,f2++,fd++)
		*fd=s1*((*f1)-(*f2))+(*f2);
	flags=0;
}
	

bool Matrix4::operator== (const Matrix4& t) const
{
	if ((flags&MATFLAG_IDENTITY)&&(t.flags&MATFLAG_IDENTITY))
	{
		assert(IntegrityCheck());
		assert(t.IntegrityCheck());
		return true;
	}
	int i,j;
	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
			if (fabs(m[i][j]-t.m[i][j])>1E-4f)
				return false;
		}
	}
	return true;
}

void Matrix4::Concat(const Matrix4 &m1,const Matrix4 &m2)
{
	if (m1.flags&MATFLAG_IDENTITY)
	{
		assert(m1.IntegrityCheck());
		*this=m2;
	}
	else if (m2.flags&MATFLAG_IDENTITY)
	{
		assert(m2.IntegrityCheck());
		*this=m1;
	}
	else
	{

		assert(this!=&m1&&this!=&m2);
	/*
		int i,j,k;
		float acc=0;
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				for(k=0,acc=0.;k<4;k++)
					acc+=m1.m[i][k]*m2.m[k][j];
				m[i][j]=acc;
			}
		}
	*/
		m2.HXFormPointND(m[0],m1.m[0]);
		m2.HXFormPointND(m[1],m1.m[1]);
		m2.HXFormPointND(m[2],m1.m[2]);
		m2.HXFormPointND(m[3],m1.m[3]);
		flags=0;
	}
}

void Matrix4::XFormPoint(Vect3 &dest,const Vect3 &src) const
{
	if (flags&MATFLAG_IDENTITY)
	{
		assert(IntegrityCheck());
		dest=src;
		return;
	}
/*
	dest[0]=m[0][0]*src[0]+m[1][0]*src[1]+m[2][0]*src[2]+m[3][0];
	dest[1]=m[0][0]*src[0]+m[1][0]*src[1]+m[2][0]*src[2]+m[3][1];
	dest[2]=m[3][2];
*/
/*
	int i,j;
	dest[0]=m[3][0];
	dest[1]=m[3][1];
	dest[2]=m[3][2];
	for (i=0;i<3;i++)
		for (j=0;j<3;j++)
			dest[i]+=m[j][i]*src[j];
*/
	const float *s=&src.x();
	float *d=&dest.x();
	const float *mat=&m[0][0];
	__asm
	{
		mov ebx,[s]
		mov eax,[mat]
		mov ecx,[d]
		fld [DWORD PTR ebx+8]  // z 
		fld [DWORD PTR ebx+4]  // y z
		fld [DWORD PTR ebx] // x y z

		fld [DWORD PTR eax]  // m00 x y z 1/m33+m23z+m13y+xm03
		fmul st,st(1) // m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+4]  // m01 m00x x y z 1/m33+m23z+m13y+xm03
		fmul st,st(2) // m01x m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+8]  // m02 m01x m00x x y z 1/m33+m23z+m13y+xm03
		fmulp st(3),st // m01x m00x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m00x m01x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(2) // m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+16]  // m10 m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+20]  // m11 m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03


		fmul st,st(5) // m11y m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m10y m11y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		faddp st(4),st // m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+24]  // m12 m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m11y m02x m01x m00x+m10y m12y z 1/m33+m23z+m13y+xm03

		faddp st(2),st // m02x m01x+m11y m00x+m10y m12y z 1/m33+m23z+m13y+xm03
		faddp st(3),st //  m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+32] // m20 m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+36] // m21 m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fmul st,st(5) // m21z m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fxch st(1) // m20z m21z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		faddp st(3),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+40] // m22 m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		faddp st(1),st // m21z+m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		fxch st(3) // m22z m00x+m10y+m20z m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03
		faddp st(2),st //  m00x+m10y+m20z m22z+m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03


		fadd [DWORD PTR eax+48] //  m00x+m10y+m20z+m30 m22z+m12y+m02x m21z+m01x+m11y 1/m33+m23z+m13y+xm03
		fxch st(2) //  m21z+m01x+m11y m22z+m12y+m02x m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fadd [DWORD PTR eax+52] // m21z+m01x+m11y+m31 m22z+m12y+m02x m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fxch st(1) // m22z+m12y+m02x m21z+m01x+m11y+m31 m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fadd [DWORD PTR eax+56] // m22z+m12y+m02x+m32 m21z+m01x+m11y+m31 m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03

		fxch st(2) // m00x+m10y+m20z+m30 m21z+m01x+m11y+m31 m22z+m12y+m02x+m32 1/m33+m23z+m13y+xm03
		fstp [DWORD PTR ecx]
		fstp [DWORD PTR ecx+4]
		fstp [DWORD PTR ecx+8]
	}
}

void Matrix4::XFormVect(Vect3 &dest,const Vect3 &src) const
{
	if (flags&MATFLAG_IDENTITY)
	{
		dest=src;
		return;
	}
	int i,j;
	dest=0.;
	for (i=0;i<3;i++)
		for (j=0;j<3;j++)
			dest[i]+=m[j][i]*src[j];
}

void Matrix4::XFormVectTranspose(Vect3 &dest,const Vect3 &src) const
{
	if (flags&MATFLAG_IDENTITY)
	{
		assert(IntegrityCheck());
		dest=src;
		return;
	}
/*
	int i,j;
	dest=0.;
	for (i=0;i<3;i++)
		for (j=0;j<3;j++)
			dest[i]+=m[i][j]*src[j];
*/
	const float *s=&src.x();
	float *d=&dest.x();
	const float *mat=&m[0][0];
	__asm
	{
		mov ebx,[s]
		mov eax,[mat]
		mov ecx,[d]
		fld [DWORD PTR ebx+8]  // z 
		fld [DWORD PTR ebx+4]  // y z
		fld [DWORD PTR ebx] // x y z

		fld [DWORD PTR eax]  // m00 x y z 1/m33+m23z+m13y+xm03
		fmul st,st(1) // m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+16]  // m01 m00x x y z 1/m33+m23z+m13y+xm03
		fmul st,st(2) // m01x m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+32]  // m02 m01x m00x x y z 1/m33+m23z+m13y+xm03
		fmulp st(3),st // m01x m00x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m00x m01x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(2) // m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+4]  // m10 m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+20]  // m11 m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03


		fmul st,st(5) // m11y m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m10y m11y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		faddp st(4),st // m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+36]  // m12 m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m11y m02x m01x m00x+m10y m12y z 1/m33+m23z+m13y+xm03

		faddp st(2),st // m02x m01x+m11y m00x+m10y m12y z 1/m33+m23z+m13y+xm03
		faddp st(3),st //  m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+8] // m20 m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+24] // m21 m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fmul st,st(5) // m21z m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fxch st(1) // m20z m21z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		faddp st(3),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+40] // m22 m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		faddp st(1),st // m21z+m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		fxch st(3) // m22z m00x+m10y+m20z m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03
		faddp st(2),st //  m00x+m10y+m20z m22z+m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03
		fstp [DWORD PTR ecx]
		fstp [DWORD PTR ecx+8]
		fstp [DWORD PTR ecx+4]
	}
}

#pragma warning( disable : 4725) //instruction may be inaccurate on some Pentiums

float Matrix4::HXFormPoint(Vect3 &dest,const Vect3 &src) const
{
/*
	int i,j;
	float h;
	h=m[3][3];
	h+=m[0][3]*src[0];
	h+=m[1][3]*src[1];
	h+=m[2][3]*src[2];
	h=1.0f/h;
	for (i=0;i<3;i++)
	{
		dest[i]=m[3][i];
		for (j=0;j<3;j++)
			dest[i]+=m[j][i]*src[j];
		dest[i]*=h;
	}
//	return h;
*/
	static const float one=1.0f;
	const float *s=&src.x();
	float *d=&dest.x();
	const float *mat=&m[0][0];
	float tret;	
	__asm
	{
		mov ebx,[s]
		mov eax,[mat]
		mov ecx,[d]
		fld [DWORD PTR ebx] // x
		fld [DWORD PTR eax+12] // m03 x
		fmul st,st(1) // xm03 x
		fld [DWORD PTR ebx+4]  // y xm03 x 
		fld [DWORD PTR eax+28]  // m13 y xm03 x 
		fmul st,st(1) // m13y y xm03 x 

		fld [DWORD PTR ebx+8]  // z m13y y xm03 x 
		fld [DWORD PTR eax+44]  // m23 z m13y y xm03 x 
		fmul st,st(1)  // m23z z m13y y xm03 x   
		fxch st(2) //  m13y z m23z y xm03 x 
		faddp st(4),st //   z m23z y m13y+xm03 x  
		fxch st(1) //  m23z z y m13y+xm03 x  		
		faddp st(3),st // z y m23z+m13y+xm03 x  		  
		fxch st(2) // m23z+m13y+xm03 y z x  		  

		fadd [DWORD PTR eax+60] // m33+m23z+m13y+xm03 y z x  
		fdivr [one] // 1/m33+m23z+m13y+xm03 y z x  
		fxch st(3) // x y z 1/m33+m23z+m13y+xm03
		mov edx,[DWORD PTR eax]
		mov edx,[DWORD PTR ecx]
		mov edx,[DWORD PTR ecx+8]



		fld [DWORD PTR eax]  // m00 x y z 1/m33+m23z+m13y+xm03
		fmul st,st(1) // m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+4]  // m01 m00x x y z 1/m33+m23z+m13y+xm03
		fmul st,st(2) // m01x m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+8]  // m02 m01x m00x x y z 1/m33+m23z+m13y+xm03
		fmulp st(3),st // m01x m00x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m00x m01x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(2) // m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+16]  // m10 m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+20]  // m11 m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03


		fmul st,st(5) // m11y m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m10y m11y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		faddp st(4),st // m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+24]  // m12 m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m11y m02x m01x m00x+m10y m12y z 1/m33+m23z+m13y+xm03

		faddp st(2),st // m02x m01x+m11y m00x+m10y m12y z 1/m33+m23z+m13y+xm03
		faddp st(3),st //  m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+32] // m20 m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+36] // m21 m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fmul st,st(5) // m21z m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fxch st(1) // m20z m21z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		faddp st(3),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+40] // m22 m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		faddp st(1),st // m21z+m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		fxch st(3) // m22z m00x+m10y+m20z m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03
		faddp st(2),st //  m00x+m10y+m20z m22z+m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03


		fadd [DWORD PTR eax+48] //  m00x+m10y+m20z+m30 m22z+m12y+m02x m21z+m01x+m11y 1/m33+m23z+m13y+xm03
		fxch st(2) //  m21z+m01x+m11y m22z+m12y+m02x m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fadd [DWORD PTR eax+52] // m21z+m01x+m11y+m31 m22z+m12y+m02x m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fxch st(1) // m22z+m12y+m02x m21z+m01x+m11y+m31 m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fadd [DWORD PTR eax+56] // m22z+m12y+m02x+m32 m21z+m01x+m11y+m31 m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03

		fxch st(2) // m00x+m10y+m20z+m30 m21z+m01x+m11y+m31 m22z+m12y+m02x+m32 1/m33+m23z+m13y+xm03
		fmul st,st(3) // sx m21z+m01x+m11y+m31 m22z+m12y+m02x+m32 1/m33+m23z+m13y+xm03
		fxch st(1) // m21z+m01x+m11y+m31 sx m22z+m12y+m02x+m32 1/m33+m23z+m13y+xm03
		fmul st,st(3) // sy sx m22z+m12y+m02x+m32 1/m33+m23z+m13y+xm03
		fxch st(2) // m22z+m12y+m02x+m32 sx sy 1/m33+m23z+m13y+xm03 
		fmul st,st(3) // sz sx sy rhw
		fstp [DWORD PTR ecx+8]
		fstp [DWORD PTR ecx]
		fstp [DWORD PTR ecx+4]
		fstp [tret]
	}
	return tret;
}
#pragma warning( default: 4725) //instruction may be inaccurate on some Pentiums

void Matrix4::HXFormPointND(float *d,const float *s) const
{
/*
	int i,j;
	float h;
	h=m[3][3];
	h+=m[0][3]*src[0];
	h+=m[1][3]*src[1];
	h+=m[2][3]*src[2];
	h=1.0f/h;
	for (i=0;i<3;i++)
	{
		dest[i]=m[3][i];
		for (j=0;j<3;j++)
			dest[i]+=m[j][i]*src[j];
		dest[i]*=h;
	}
//	return h;
*/
	const float *mat=&m[0][0];
	__asm
	{
		mov ebx,[s]
		mov eax,[mat]
		mov ecx,[d]
		fld [DWORD PTR ebx] // x
		fld [DWORD PTR eax+12] // m03 x
		fmul st,st(1) // xm03 x
		fld [DWORD PTR ebx+4]  // y xm03 x 
		fld [DWORD PTR eax+28]  // m13 y xm03 x 
		fmul st,st(1) // m13y y xm03 x 

		fld [DWORD PTR ebx+8]  // z m13y y xm03 x 
		fld [DWORD PTR eax+44]  // m23 z m13y y xm03 x 
		fmul st,st(1)  // m23z z m13y y xm03 x   
		fxch st(2) //  m13y z m23z y xm03 x 
		faddp st(4),st //   z m23z y m13y+xm03 x  
		fxch st(1) //  m23z z y m13y+xm03 x  		
		faddp st(3),st // z y m23z+m13y+xm03 x  		  
		fxch st(2) // m23z+m13y+xm03 y z x  		  
		fld [DWORD PTR ebx+12] // m33+m23z+m13y+xm03 y z x  
		fmul [DWORD PTR eax+60] // m33+m23z+m13y+xm03 y z x  
		faddp st(1),st
		fxch st(3) // x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax]  // m00 x y z 1/m33+m23z+m13y+xm03
		fmul st,st(1) // m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+4]  // m01 m00x x y z 1/m33+m23z+m13y+xm03
		fmul st,st(2) // m01x m00x x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+8]  // m02 m01x m00x x y z 1/m33+m23z+m13y+xm03
		fmulp st(3),st // m01x m00x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m00x m01x m02x y z 1/m33+m23z+m13y+xm03
		fxch st(2) // m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+16]  // m10 m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+20]  // m11 m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03


		fmul st,st(5) // m11y m10y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		fxch st(1) // m10y m11y m02x m01x m00x y z 1/m33+m23z+m13y+xm03
		faddp st(4),st // m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+24]  // m12 m11y m02x m01x m00x+m10y y z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m11y m02x m01x m00x+m10y m12y z 1/m33+m23z+m13y+xm03

		faddp st(2),st // m02x m01x+m11y m00x+m10y m12y z 1/m33+m23z+m13y+xm03
		faddp st(3),st //  m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+32] // m20 m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fmul st,st(4) // m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+36] // m21 m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03

		fmul st,st(5) // m21z m20z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		fxch st(1) // m20z m21z m01x+m11y m00x+m10y m12y+m02x z 1/m33+m23z+m13y+xm03
		faddp st(3),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+40] // m22 m21z m01x+m11y m00x+m10y+m20z m12y+m02x z 1/m33+m23z+m13y+xm03
		fmulp st(5),st // m21z m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		faddp st(1),st // m21z+m01x+m11y m00x+m10y+m20z m12y+m02x m22z 1/m33+m23z+m13y+xm03
		fxch st(3) // m22z m00x+m10y+m20z m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03
		faddp st(2),st //  m00x+m10y+m20z m22z+m12y+m02x m21z+m01x+m11y  1/m33+m23z+m13y+xm03


//		fadd [DWORD PTR eax+48] //  m00x+m10y+m20z+m30 m22z+m12y+m02x m21z+m01x+m11y 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+48]
		fmul [DWORD PTR ebx+12] 
		faddp st(1),st
		fxch st(2) //  m21z+m01x+m11y m22z+m12y+m02x m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03

		fld [DWORD PTR eax+52]
		fmul [DWORD PTR ebx+12] 
		faddp st(1),st
		fxch st(1) // m22z+m12y+m02x m21z+m01x+m11y+m31 m00x+m10y+m20z+m30 1/m33+m23z+m13y+xm03
		fld [DWORD PTR eax+56]
		fmul [DWORD PTR ebx+12] 
		faddp st(1),st

		fstp [DWORD PTR ecx+8]
		fstp [DWORD PTR ecx+4]
		fstp [DWORD PTR ecx]
		fstp [DWORD PTR ecx+12]
	}
}



void Matrix4::From3x4(const float mat[3][4])
{
	for (int i=0; i<4; i++)
	{
		for (int j=0; j<3; j++)
		{
			m[i][j]=mat[j][i];
		}
	}

	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][3] = 1.0f;

	CalcFlags();
}

void Matrix4::To3x4(float mat[3][4]) const
{
	for (int i=0; i<4; i++)
	{
		for (int j=0; j<3; j++)
		{
			mat[j][i] = m[i][j];
		}
	}
}

void Matrix4::SetRow(int i,const Vect3 &t)
{
	m[i][0]=t[0];
	m[i][1]=t[1];
	m[i][2]=t[2];
	flags=0;
}

void Matrix4::SetColumn(int i,const Vect3 &t)
{
	m[0][i]=t[0];
	m[1][i]=t[1];
	m[2][i]=t[2];
	flags=0;
}

void Matrix4::SetRow(int i)
{
	m[i][0]=0;
	m[i][1]=0;
	m[i][2]=0;
	flags=0;
}

void Matrix4::SetColumn(int i)
{
	m[0][i]=0;
	m[1][i]=0;
	m[2][i]=0;
	flags=0;
}

void Matrix4::SetFromDouble(double *d)
{
	int i;
	float *f;
	for (f=m[0],i=0;i<16;i++,f++,d++)
		*f=(float)*d;
	flags=0;
}


void Matrix4::MultiplyColumn(int i,float f)
{
	m[0][i]*=f;
	m[1][i]*=f;
	m[2][i]*=f;
	flags=0;
}

float Matrix4::GetColumnLen(int i)
{
	float d;
	d=m[0][i]*m[0][i]+m[1][i]*m[1][i]+m[2][i]*m[2][i];
	assert(d>EPS_MATRIX);
	return sqrt(d);
}

void Matrix4::Transpose()
{
	int i,j;
	float swap;
	for (i=0;i<4;i++)
	{
		for (j=i+1;j<4;j++)
		{
			swap=m[i][j];
			m[i][j]=m[j][i];
			m[j][i]=swap;
		}
	}
}

