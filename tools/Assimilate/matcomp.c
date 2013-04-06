#include "MatComp.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <windows.h>

#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_N_VECT (1<<(MC_BITS_VECT))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V1 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V1 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V2 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V2 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V3 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V3 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MAXBIT 30
#define MAXDIM 6

void sobseq(int n, int *x)
{
	int j,k,l;
	unsigned int i,im,ipp;
	static unsigned int in,ix[MAXDIM+1],*iu[MAXBIT+1];
	static unsigned int mdeg[MAXDIM+1]={0,1,2,3,3,4,4};
	static unsigned int ip[MAXDIM+1]={0,0,1,1,2,1,4};
	static unsigned int iv[MAXDIM*MAXBIT+1]={
		0,1,1,1,1,1,1,3,1,3,3,1,1,5,7,7,3,3,5,15,11,5,15,13,9};

	if (n < 0) {
		for (j=1,k=0;j<=MAXBIT;j++,k+=MAXDIM) iu[j] = &iv[k];
		for (k=1;k<=MAXDIM;k++) {
			for (j=1;j<=(int)mdeg[k];j++) iu[j][k] <<= (MAXBIT-j);
			for (j=mdeg[k]+1;j<=MAXBIT;j++) {
				ipp=ip[k];
				i=iu[j-mdeg[k]][k];
				i ^= (i >> mdeg[k]);
				for (l=mdeg[k]-1;l>=1;l--) {
					if (ipp & 1) i ^= iu[j-l][k];
					ipp >>= 1;
				}
				iu[j][k]=i;
			}
		}
		in=0;
	} else {
		im=in;
		for (j=1;j<=MAXBIT;j++) {
			if (!(im & 1)) break;
			im >>= 1;
		}
		assert(j <= MAXBIT);
		im=(j-1)*MAXDIM;
		for (k=1;k<=n;k++) {
			ix[k] ^= iv[im+k];
			x[k-1]=(int)(ix[k]>>14)-32768;
		}
		in++;
	}
}

static float MC_Norms[MC_N_VECT][3];
static int MC_Inited=0;

void MC_Init()
{
	int x[3];
	int sq,num=0;
	int tnum=0;
	float d;
	MC_Inited=1;
	sobseq(-3,0);
	while (num<MC_N_VECT)
	{
		tnum++;
		sobseq(3,x);
		sq=x[0]*x[0]+x[1]*x[1]+x[2]*x[2];
		if (sq<16000*16000||sq>17000*17000)
			continue;
		d=1.0f/(float)sqrt((float)sq);
		MC_Norms[num][0]=d*((float)(x[0]));
		MC_Norms[num][1]=d*((float)(x[1]));
		MC_Norms[num][2]=d*((float)(x[2]));
		num++;
	}
#if 0
	printf("%d trys to get %d vals\n",tnum,num);
	{
		int i,j;
		float mn=5.0f,mx;
		for (i=0;i<num;i++)
		{
			mx=0.0f;
			for (j=0;j<num;j++)
			{
				if (i!=j)
				{
					d=MC_Norms[i][0]*MC_Norms[j][0]+MC_Norms[i][1]*MC_Norms[j][1]+MC_Norms[i][2]*MC_Norms[j][2];
					if (d>mx)
						mx=d;
				}
			}
			if (mx<mn)
				mn=mx;
		}
		printf("minimum dot %f, ang = %f\n",mn,acos(mn)*180.0f/3.1415);
	}
#endif
}

int MC_Find(const float v[3])
{
	int i;
	float d,mx=0.0f;
	int mxat=0;

	for (i=0;i<MC_N_VECT;i++)
	{
		d=v[0]*MC_Norms[i][0]+v[1]*MC_Norms[i][1]+v[2]*MC_Norms[i][2];
		if (d>mx)
		{
			mx=d;
			mxat=i;
		}
	}
	return mxat;
}

void MC_Compress(const float mat[3][4],unsigned char * comp)
{
	int i,val;
	if (!MC_Inited)
		MC_Init();
	for (i=0;i<MC_COMP_BYTES;i++)
		comp[i]=0;

	val=(int)(mat[0][3]/MC_SCALE_X);
	val+=1<<(MC_BITS_X-1);
	if (val>=(1<<MC_BITS_X))
		val=(1<<MC_BITS_X)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_X)|=((unsigned int)(val))<<MC_SHIFT_X;

	val=(int)(mat[1][3]/MC_SCALE_Y);
	val+=1<<(MC_BITS_Y-1);
	if (val>=(1<<MC_BITS_Y))
		val=(1<<MC_BITS_Y)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_Y)|=((unsigned int)(val))<<MC_SHIFT_Y;

	val=(int)(mat[2][3]/MC_SCALE_Z);
	val+=1<<(MC_BITS_Z-1);
	if (val>=(1<<MC_BITS_Z))
		val=(1<<MC_BITS_Z)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_Z)|=((unsigned int)(val))<<MC_SHIFT_Z;

	val=MC_Find(mat[0]);
	*(unsigned int *)(comp+MC_POS_V1)|=((unsigned int)(val))<<MC_SHIFT_V1;

	val=MC_Find(mat[1]);
	*(unsigned int *)(comp+MC_POS_V2)|=((unsigned int)(val))<<MC_SHIFT_V2;

	val=MC_Find(mat[2]);
	*(unsigned int *)(comp+MC_POS_V3)|=((unsigned int)(val))<<MC_SHIFT_V3;
}

void MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val;
	unsigned int uval;
	if (!MC_Inited)
		MC_Init();

	uval=*(unsigned int *)(comp+MC_POS_X);
	uval>>=MC_SHIFT_X;
	uval&=MC_MASK_X;
	val=(int)uval;
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	uval=*(unsigned int *)(comp+MC_POS_Y);
	uval>>=MC_SHIFT_Y;
	uval&=MC_MASK_Y;
	val=(int)uval;
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	uval=*(unsigned int *)(comp+MC_POS_Z);
	uval>>=MC_SHIFT_Z;
	uval&=MC_MASK_Z;
	val=(int)uval;
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	uval=*(unsigned int *)(comp+MC_POS_V1);
	uval>>=MC_SHIFT_V1;
	uval&=MC_MASK_VECT;
	mat[0][0]=MC_Norms[uval][0];
	mat[0][1]=MC_Norms[uval][1];
	mat[0][2]=MC_Norms[uval][2];

	uval=*(unsigned int *)(comp+MC_POS_V2);
	uval>>=MC_SHIFT_V2;
	uval&=MC_MASK_VECT;
	mat[1][0]=MC_Norms[uval][0];
	mat[1][1]=MC_Norms[uval][1];
	mat[1][2]=MC_Norms[uval][2];

	uval=*(unsigned int *)(comp+MC_POS_V3);
	uval>>=MC_SHIFT_V3;
	uval&=MC_MASK_VECT;
	mat[2][0]=MC_Norms[uval][0];
	mat[2][1]=MC_Norms[uval][1];
	mat[2][2]=MC_Norms[uval][2];

}

