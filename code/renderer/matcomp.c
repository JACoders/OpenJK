/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#include "MatComp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>	// for memcpy

#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT (1.0f/(float)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))

void MC_Compress(const float mat[3][4],unsigned char * _comp)
{
	char comp[MC_COMP_BYTES*2];

	int i,val;
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


	val=(int)(mat[0][0]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V11)|=((unsigned int)(val))<<MC_SHIFT_V11;

	val=(int)(mat[0][1]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V12)|=((unsigned int)(val))<<MC_SHIFT_V12;

	val=(int)(mat[0][2]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V13)|=((unsigned int)(val))<<MC_SHIFT_V13;


	val=(int)(mat[1][0]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V21)|=((unsigned int)(val))<<MC_SHIFT_V21;

	val=(int)(mat[1][1]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V22)|=((unsigned int)(val))<<MC_SHIFT_V22;

	val=(int)(mat[1][2]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V23)|=((unsigned int)(val))<<MC_SHIFT_V23;

	val=(int)(mat[2][0]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V31)|=((unsigned int)(val))<<MC_SHIFT_V31;

	val=(int)(mat[2][1]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V32)|=((unsigned int)(val))<<MC_SHIFT_V32;

	val=(int)(mat[2][2]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V33)|=((unsigned int)(val))<<MC_SHIFT_V33;

	// I added this because the line above actually ORs data into an int at the 22 byte (from 0), and therefore technically
	//	is writing beyond the 24th byte of the output array. This *should** be harmless if the OR'd-in value doesn't change 
	//	those bytes, but BoundsChecker says that it's accessing undefined memory (which it does, sometimes). This is probably
	//	bad, so... 
	memcpy(_comp,comp,MC_COMP_BYTES);
}

void MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val;

	val=(int)((unsigned short *)(comp))[0];
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	val=(int)((unsigned short *)(comp))[1];
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	val=(int)((unsigned short *)(comp))[2];
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	val=(int)((unsigned short *)(comp))[3];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[4];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[5];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[6];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[7];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[8];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[9];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[10];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[11];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][2]=((float)(val))*MC_SCALE_VECT;

}

/* ROTATION/TRANSLATION COMPRESSION DEBUG CODE (RTCDC)
unsigned short comp_rot_maskx = 0xfff0;
unsigned short comp_rot_masky = 0xfff0;
unsigned short comp_rot_maskz = 0xfff0;
unsigned short comp_rot_maskw = 0xfff0;

unsigned short comp_tra_maskf1 = 0xfff0;
unsigned short comp_tra_maskf2 = 0xfff0;
unsigned short comp_tra_maskf3 = 0xfff0;

int use_comp_rot_mask = 0;
int use_comp_tra_mask = 0;
unsigned short cw;
unsigned short cx;
unsigned short cy;
unsigned short cz;
unsigned short f1;
unsigned short f2;
unsigned short f3;
*/

void MC_UnCompressQuat(float mat[3][4],const unsigned char * comp)
{
	float w,x,y,z,f;
    float fTx;
    float fTy;
    float fTz;
    float fTwx;
    float fTwy;
    float fTwz;
    float fTxx;
    float fTxy;
    float fTxz;
    float fTyy;
    float fTyz;
    float fTzz;
	
	const unsigned short *pwIn = (unsigned short *) comp;
	

	/* RTCDC
	if(use_comp_rot_mask)
	{
		cw = *pwIn++;
		cx = *pwIn++;
		cy = *pwIn++;
		cz = *pwIn++;

		cw &= comp_rot_maskw;
		cx &= comp_rot_maskx;
		cy &= comp_rot_masky;
		cz &= comp_rot_maskz;
	}
	else
	{
		cw = *pwIn++;
		cx = *pwIn++;
		cy = *pwIn++;
		cz = *pwIn++;
	}

	if(use_comp_tra_mask)
	{
		f1 = *pwIn++;
		f2 = *pwIn++;
		f3 = *pwIn++;

		f1 &= comp_tra_maskf1;
		f2 &= comp_tra_maskf2;
		f3 &= comp_tra_maskf3;
	}
	else
	{
		f1 = *pwIn++;
		f2 = *pwIn++;
		f3 = *pwIn++;
	}
	*/

	w = *pwIn++;
	// RTCDC w = cw;
	w/=16383.0f;
	w-=2.0f;
	x = *pwIn++;
	// RTCDC x = cx;
	x/=16383.0f;
	x-=2.0f;
	y = *pwIn++;
	// RTCDC y = cy;
	y/=16383.0f;
	y-=2.0f;
	z = *pwIn++;
	// RTCDC z = cz;
	z/=16383.0f;
	z-=2.0f;

    fTx  = 2.0f*x;
    fTy  = 2.0f*y;
    fTz  = 2.0f*z;
    fTwx = fTx*w;
    fTwy = fTy*w;
    fTwz = fTz*w;
    fTxx = fTx*x;
    fTxy = fTy*x;
    fTxz = fTz*x;
    fTyy = fTy*y;
    fTyz = fTz*y;
    fTzz = fTz*z;

	// rot...
	//
    mat[0][0] = 1.0f-(fTyy+fTzz);
    mat[0][1] = fTxy-fTwz;
    mat[0][2] = fTxz+fTwy;
    mat[1][0] = fTxy+fTwz;
    mat[1][1] = 1.0f-(fTxx+fTzz);
    mat[1][2] = fTyz-fTwx;
    mat[2][0] = fTxz-fTwy;
    mat[2][1] = fTyz+fTwx;
    mat[2][2] = 1.0f-(fTxx+fTyy);

	// xlat...
	//
	f = *pwIn++;
	// RTCDC f = f1;
	f/=64;
	f-=512;
	mat[0][3] = f;

	f = *pwIn++;
	// RTCDC f = f2;
	f/=64;
	f-=512;
	mat[1][3] = f;

	f = *pwIn++;
	// RTCDC f = f3;
	f/=64;
	f-=512;
	mat[2][3] = f;
}


